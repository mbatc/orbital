#include "util/ThreadPool.h"
#include "core/Pool.h"

namespace bfc {
  namespace impl {
    static thread_local bool isPooledThread = false;

    class PooledRunner {
    public:
      PooledRunner(int64_t numThreads)
      {
        for (int64_t i = 0; i < numThreads; ++i)
          m_threads.pushBack(std::thread(&PooledRunner::worker, this));
      }

      ~PooledRunner() {
        m_lock.lock();
        m_running = false;
        m_lock.unlock();

        m_notifier.notify_all();

        for (auto & th : m_threads)
          th.join();
      }

      void add(Vector<ThreadPool::Task> &&tasks) {
        m_lock.lock();
        for (auto & task : tasks) {
          m_queue.pushBack(std::move(task));
        }
        m_lock.unlock();

        for (int64_t i = 0; i < tasks.size(); ++i) {
          m_notifier.notify_one();
        }

        tasks.clear();
      }

      int64_t availableToRun() const {
        std::unique_lock guard{m_lock};
        return m_threads.size() - m_queue.size() - m_numBusy;
      }

    private:
      void worker() {
        isPooledThread = true;

        std::optional<ThreadPool::Task> task;

        bool running = true;
        while (running) {
          {
            std::unique_lock ul{m_lock};
            m_notifier.wait(ul, [&]() {
              running = m_running || m_queue.size() > 0;

              if (m_queue.size() == 0)
                return !m_running;

              task = m_queue.popFront();
              m_numBusy++;
              return true;
            });
          }

          if (!task.has_value())
            continue;

          task->callback();
          task.reset();
          --m_numBusy;
        }
      }

      bool                     m_running = true;
      Vector<std::thread>      m_threads;
      Vector<ThreadPool::Task> m_queue;
      mutable std::mutex       m_lock;
      std::condition_variable  m_notifier;
      std::atomic_int64_t      m_numBusy = 0;
    };

    class ThreadedRunner {
    public:
      ThreadedRunner() {
        m_cleanup = std::thread(&ThreadedRunner::cleanup, this);
      }

      ~ThreadedRunner() {
        m_threadLock.lock();
        m_running = false;
        m_threadLock.unlock();
        m_threadNotifier.notify_one();

        m_cleanup.join();
      }

      bool run(ThreadPool::Task task) {
        std::unique_lock guard{m_threadLock};
        if (!m_running) {
          return false;
        }

        int64_t index = m_active.emplace();
        m_active[index] = std::thread(([this, index, task = std::move(task)]() {
          isPooledThread = true;

          task.callback();

          m_threadLock.lock();
          m_dead.pushBack(std::move(m_active[index]));
          m_active.erase(index);
          m_threadLock.unlock();
          m_threadNotifier.notify_one();
        }));

        return true;
      }

    private:
      // joins finished threads
      void cleanup() {
        Vector<std::thread> joinable;
        bool                running = true;
        while (running) {
          std::unique_lock ul{m_threadLock};
          m_threadNotifier.wait(ul, [&]() {
            running  = m_running || (m_active.size() + m_dead.size()) > 0;
            joinable = std::move(m_dead);
            m_dead.clear();
            return !running || joinable.size() > 0;
          });

          for (auto & th : joinable)
            th.join();
          joinable.clear();
        }
      }

      bool                    m_running = true;
      std::mutex              m_threadLock;
      std::condition_variable m_threadNotifier;
      Pool<std::thread>       m_active;
      Vector<std::thread>     m_dead;
      std::thread m_cleanup;
    };
  } // namespace impl

  ThreadPool::ThreadPool(int64_t targetConcurrency) {
    m_dispatcher = std::thread(&ThreadPool::dispatchTasks, this, targetConcurrency);
  }

  ThreadPool::~ThreadPool() {
    m_lock.lock();
    m_running = false;
    m_lock.unlock();

    m_dispatchNotifier.notify_one();
    m_dispatcher.join();
  }

  bool ThreadPool::IsPoolThread() {
    return false;
  }

  ThreadPool & ThreadPool::Global() {
    static ThreadPool instance;
    return instance;
  }

  void ThreadPool::dispatchTasks(int64_t targetConcurrency) {
    impl::ThreadedRunner runner;
    impl::PooledRunner   workers(targetConcurrency);

    Vector<Task>     toWorkers;
    Vector<Task>     alwaysRun;
    Vector<Task>     newThreads;
    bool             running = true;

    while (running) {
      {
        std::unique_lock ul{m_lock};
        m_dispatchNotifier.wait(ul, [&]() {
          running = m_running || m_dispatchQueue.size() > 0;

          if (m_dispatchQueue.empty())
            return !m_running;

          for (auto & task : m_dispatchQueue) {
            if (task.flags & AsyncFlags_AlwaysRun)
              alwaysRun.pushBack(std::move(task));
            else if (task.flags & AsyncFlags_NewThread)
              newThreads.pushBack(std::move(task));
            else
              toWorkers.pushBack(std::move(task));
          }
          m_dispatchQueue.clear();
          return true;
        });
      }

      int64_t freeWorkers = workers.availableToRun() - toWorkers.size();

      // Distribute always run tasks  to workers or to the threaded runner.
      for (auto & task : alwaysRun) {
        if (freeWorkers <= 0) {
          newThreads.pushBack(std::move(task));
        } else {
          toWorkers.pushBack(std::move(task));
          --freeWorkers;
        }
      }

      // Dispatch to workers
      workers.add(std::move(toWorkers));

      // Dispatch to new thread
      for (auto & task : newThreads)
        runner.run(std::move(task));

      toWorkers.clear();
      alwaysRun.clear();
      newThreads.clear();
    }
  }
} // namespace bfc

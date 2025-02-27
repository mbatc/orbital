#pragma once

#include "core/Vector.h"

#include <future>
#include <optional>

namespace bfc {
  enum AsyncFlags {
    AsyncFlags_None      = 0,      ///< Default behaviour. Run in a pooled thread.
    AsyncFlags_NewThread = 1 << 0, ///< Always run in a new thread.
    AsyncFlags_AlwaysRun = 1 << 1, ///< Start a new thread if no pooled threads are available.
  };

  class BFC_API ThreadPool {
  public:

    struct Task {
      std::function<void()> callback;
      AsyncFlags            flags;
    };

    ThreadPool(int64_t targetConcurrency = std::thread::hardware_concurrency());

    ~ThreadPool();

    /// Get the global thread pool instance.
    static ThreadPool & Global();

    template<typename Callable, typename... Args>
    auto run(Callable&& cb, Args &&... args) -> std::future<return_value_of_t<Callable, Args...>> {
      return run<Callable, Args...>(AsyncFlags_None, std::forward<Callable>(cb), std::forward<Args>(args)...);
    }

    template<typename Callable, typename... Args>
    auto run(AsyncFlags flags, Callable && cb, Args &&... args) -> std::future<return_value_of_t<Callable, Args...>> {
      using R                = typename function_type<Callable>::Ret;
      auto           promise = std::make_shared<std::promise<R>>();
      std::future<R> future  = promise->get_future();

      Task task;
      task.flags    = flags;
      task.callback = [cb = std::forward<Callable>(cb), args = std::make_tuple(std::forward<Args>(args)...), promise]() mutable {
        if constexpr (std::is_same_v<R, void>) {
          bfc::invoke(cb, std::move(args));
          promise->set_value();
        } else {
          promise->set_value(bfc::invoke(cb, std::move(args)));
        }
      };

      {
        std::scoped_lock guard{m_lock};

        if (!m_running) {
          promise->set_exception(std::make_exception_ptr(std::exception("Thread pool is not running")));
          return future;
        }

        m_dispatchQueue.pushBack(std::move(task));
      }

      m_dispatchNotifier.notify_one();

      return future;
    }

  private:
    void dispatchTasks(int64_t targetConcurrency = std::thread::hardware_concurrency());

    std::mutex              m_lock;
    std::condition_variable m_dispatchNotifier;
    Vector<Task>            m_dispatchQueue;
    bool                    m_running = true;

    std::thread                   m_dispatcher;
  };

  template<>
  struct enable_bitwise_operators<AsyncFlags> : std::true_type {};

  template<typename Callable, typename... Args>
  auto async(Callable && cb, Args &&... args) -> std::future<return_value_of_t<Callable, Args...>> {
    return async(AsyncFlags_None, std::forward<Callable>(cb), std::forward<Args>(args)...);
  }

  template<typename Callable, typename... Args>
  auto async(AsyncFlags flags, Callable && cb, Args &&... args) -> std::future<return_value_of_t<Callable, Args...>> {
    return ThreadPool::Global().run(flags, std::forward<Callable>(cb), std::forward<Args>(args)...);
  }
}

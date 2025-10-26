#include "framework/test.h"
#include "util/ThreadPool.h"
#include <chrono>

using namespace bfc;
using namespace std::chrono_literals;

BFC_TEST(ThreadPool_Pooled) {
  ThreadPool                   threads;
  Vector<std::future<int64_t>> results;

  for (int64_t i = 0; i < 100; ++i)
    results.pushBack(threads.run([i]() { return i; }));

  for (int64_t i = 0; i < results.size(); ++i) {
    BFC_TEST_ASSERT_EQUAL(results[i].wait_for(1s), std::future_status::ready);
    BFC_TEST_ASSERT_EQUAL(results[i].get(), i);
  }
}

BFC_TEST(ThreadPool_AlwaysRun) {
  ThreadPool                   threads(1);

  std::mutex              lock;
  std::condition_variable notifier;

  std::future<std::cv_status> status = threads.run([&]() {
    std::unique_lock guard{lock};
    return notifier.wait_for(guard, 5s);
  });

  Vector<std::future<int64_t>> results;
  for (int64_t i = 0; i < 10; ++i)
    results.pushBack(threads.run(AsyncFlags_AlwaysRun,
      [i]() {
          return i;
        }));

  for (int64_t i = 0; i < results.size(); ++i) {
    auto val = results[i].wait_for(1s);

    BFC_TEST_ASSERT_EQUAL(results[i].wait_for(1s), std::future_status::ready);
    BFC_TEST_ASSERT_EQUAL(results[i].get(), i);
  }

  notifier.notify_one();

  BFC_TEST_ASSERT_EQUAL(status.get(), std::cv_status::no_timeout);
}

BFC_TEST(ThreadPool_NewThread) {
  ThreadPool threads(1);

  std::mutex              lock;
  std::condition_variable notifier;

  std::future<std::cv_status> status = threads.run([&]() {
    std::unique_lock guard{lock};
    return notifier.wait_for(guard, 5s);
  });

  Vector<std::future<int64_t>> results;
  for (int64_t i = 0; i < 10; ++i)
    results.pushBack(threads.run(AsyncFlags_NewThread, [i]() { return i; }));

  for (int64_t i = 0; i < results.size(); ++i) {
    auto val = results[i].wait_for(1s);

    BFC_TEST_ASSERT_EQUAL(results[i].wait_for(1s), std::future_status::ready);
    BFC_TEST_ASSERT_EQUAL(results[i].get(), i);
  }

  notifier.notify_one();

  BFC_TEST_ASSERT_EQUAL(status.get(), std::cv_status::no_timeout);
}

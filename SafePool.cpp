#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <queue>
#pragma once
// 重构后的单任务队列线程池
template <typename T> class SafePool {
public:
  template <typename V> void push(V &&x) {
    {
      std::scoped_lock<std::mutex> lock(queue_mutex);
      queue_.push(std::forward<V>(x));
    }
    cond_.notify_one();
  }

  bool pop(T &x) {
    std::unique_lock<std::mutex> lock(queue_mutex);
    cond_.wait(lock, [&]() { return !queue_.empty() || stop; });

    if (queue_.empty()) {
      return false;
    }
    x = std::move(queue_.front());
    queue_.pop();
    return true;
  }
  std::size_t size() {
    std::scoped_lock<std::mutex> lock(queue_mutex);
    return queue_.size();
  }
  std::size_t empty() {
    std::scoped_lock<std::mutex> lock(queue_mutex);
    return queue_.empty();
  }

  void stop_thread() {
    {
      std::scoped_lock<std::mutex> lock(queue_mutex);
      stop = true;
    }
    cond_.notify_all();
  }

private:
  std::mutex queue_mutex;
  std::condition_variable cond_;
  std::queue<T> queue_;
  bool stop{false};
};
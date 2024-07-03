#include <functional>
#include <future>
#include <queue>
#include <stdexcept>
#include <thread>
#include <type_traits>
#pragma once
// 未重构的单任务队列线程池
class ThreadPool {
public:
  explicit ThreadPool(
      std::size_t threads_cnt = std::thread::hardware_concurrency()) {
    for (size_t i = 0; i < threads_cnt; i++) {
      workers.emplace_back([this]() {
        for (;;) {
          std::function<void()> task;
          {
            std::unique_lock<std::mutex> lock(this->queue_mutex);
            this->condition.wait(
                lock, [this]() { return this->stop || !this->tasks.empty(); });
            if (stop && tasks.empty()) {
              return;
            }
            task = std::move(tasks.front());
            tasks.pop();
          }
          task();
        }
      });
    }
  }

  template <typename F, typename... Args> auto enqueue(F &&f, Args &&...args) {
    using return_type = std::invoke_result_t<F, Args...>;
    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...));
    std::future<return_type> res = task->get_future();
    {
      std::unique_lock<std::mutex> lock(queue_mutex);
      if (stop) {
        throw std::runtime_error("enqueue is stopped on thread pool");
      }
      tasks.emplace([task = std::move(task)] { (*task)(); });
    }
    // 注意在有队伍进入队列时，仅需要notify_one()，避免无意义的线程竞争
    condition.notify_one();
    return res;
  }
  ~ThreadPool() {
    {
      std::unique_lock<std::mutex> lock(queue_mutex);
      stop = true;
    }
    condition.notify_all();
    for (auto &work : workers) {
      work.join();
    }
  }

private:
  std::vector<std::thread> workers;
  std::queue<std::function<void()>> tasks;
  std::mutex queue_mutex;
  std::condition_variable condition;
  bool stop{false};
};
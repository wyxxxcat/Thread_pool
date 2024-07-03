#include "SafePool.cpp"
#include <cstddef>
#include <functional>
#include <thread>
#pragma once
class SimplePool {
public:
  using Item = std::function<void()>;
  explicit SimplePool(size_t thread_cnt = std::thread::hardware_concurrency()) {
    for (size_t i = 0; i < thread_cnt; i++) {
      workers.emplace_back([this]() {
        while (1) {
          Item task;
          if (!queue_.pop(task)) {
            return;
          }
          if (task) {
            task();
          }
        }
      });
    }
  }
  void enqueue(Item it) { queue_.push(std::move(it)); }
  ~SimplePool() {
    queue_.stop_thread();
    for (auto &work : workers) {
      work.join();
    }
  }

private:
  SafePool<Item> queue_;
  std::vector<std::thread> workers;
};
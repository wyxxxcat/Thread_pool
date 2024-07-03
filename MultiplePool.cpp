#include "SafePool.cpp"
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <thread>
#pragma once
// 多任务队列线程池
class MultiplePool {
public:
  using Item = std::function<void()>;
  explicit MultiplePool(size_t thread_num = std::thread::hardware_concurrency())
      : queue_(thread_num), thread_num_(thread_num) {
    auto work = [&](size_t id) {
      while (true) {
        Item task{};
        if (!queue_[id].pop(task)) {
          break;
        }
        if (task) {
          task();
        }
      }
    };
    workers_.reserve(thread_num_);
    for (size_t i = 0; i < thread_num_; i++) {
      workers_.emplace_back(work, i);
    }
  }
  int schedule(Item it, int id = -1) {
    if (it == nullptr) {
      return -1;
    }
    if (id == -1) {
      id = rand() % thread_num_;
      queue_[id].push(std::move(it));
    } else {
      assert(id < thread_num_);
      queue_[id].push(std::move(it));
    }
    return 0;
  }

  ~MultiplePool() {
    for (auto &it : queue_) {
      it.stop_thread();
    }
    for (auto &it : workers_) {
      it.join();
    }
  }

private:
  size_t thread_num_;
  std::vector<SafePool<Item>> queue_;
  std::vector<std::thread> workers_;
};
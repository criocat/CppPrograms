#pragma once

#include <cstddef>

struct control_block {
  virtual ~control_block() = default;
  virtual void delete_data() noexcept = 0;

  control_block()
      : weak_count_(0)
      , strong_count_(1) {}

  void suicide() {
    delete this;
  }

  void decrease_strong() noexcept {
    if (--strong_count_ == 0) {
      delete_data();
      if (weak_count_ == 0) {
        suicide();
      }
    }
  }

  void decrease_weak() noexcept {
    if (--weak_count_ == 0 && strong_count_ == 0) {
      suicide();
    }
  }

  void increase_strong() noexcept {
    strong_count_++;
  }

  void increase_weak() noexcept {
    weak_count_++;
  }

  std::size_t get_strong_count() const noexcept {
    return strong_count_;
  }

private:
  std::size_t weak_count_;
  std::size_t strong_count_;
};

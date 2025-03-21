#pragma once

#include <control_block.h>

#include <memory>

template <typename T>
struct control_block_obj : control_block {
  template <typename... Args>
  explicit control_block_obj(Args&&... args)
      : obj_(std::forward<Args>(args)...) {}

  ~control_block_obj() override {}

  void delete_data() noexcept override {
    obj_.~T();
  }

  T* get_data() noexcept {
    return &obj_;
  }

private:
  union {
    T obj_;
  };
};

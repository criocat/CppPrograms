#pragma once

#include <control_block.h>

#include <memory>

template <typename T, typename Deleter = std::default_delete<T>>
struct control_block_ptr : control_block {
public:
  control_block_ptr(T* ptr, Deleter del)
      : data_(ptr)
      , deleter(std::move(del)) {}

  explicit control_block_ptr(T* ptr)
      : data_(ptr) {}

  ~control_block_ptr() override = default;

  void delete_data() noexcept override {
    deleter(data_);
  }

private:
  T* data_;
  [[no_unique_address]] Deleter deleter;
};

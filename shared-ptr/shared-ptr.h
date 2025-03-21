#pragma once

#include <control_block.h>
#include <control_block_obj.h>
#include <control_block_ptr.h>

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

template <typename T>
class shared_ptr {
public:
  friend void swap(shared_ptr& l, shared_ptr& r) noexcept {
    std::swap(l.cb_, r.cb_);
    std::swap(l.ptr_, r.ptr_);
  }
  template <typename Y>
  friend class weak_ptr;
  template <typename Y>
  friend class shared_ptr;
  template <typename Y, typename... Args>
  friend shared_ptr<Y> make_shared(Args&&... args);

public:
  shared_ptr() noexcept
      : ptr_(nullptr)
      , cb_(nullptr) {}

  shared_ptr(std::nullptr_t) noexcept
      : shared_ptr() {}

  template <typename Y>
    requires std::is_convertible_v<Y*, T*>
  explicit shared_ptr(Y* ptr)
      : shared_ptr(ptr, std::default_delete<Y>()) {}

  template <typename Y, typename Deleter = std::default_delete<Y>>
    requires std::is_convertible_v<Y*, T*>
  shared_ptr(Y* ptr, Deleter deleter)
      : ptr_(static_cast<T*>(ptr)) {
    try {
      cb_ = new control_block_ptr<Y, Deleter>(ptr, std::move(deleter));
    } catch (...) {
      deleter(ptr);
      throw;
    }
  }

  template <typename Y>
  shared_ptr(const shared_ptr<Y>& other, T* ptr) noexcept
      : shared_ptr(other.cb_, ptr) {}

  template <typename Y>
  shared_ptr(shared_ptr<Y>&& other, T* ptr) noexcept
      : shared_ptr(std::exchange(other.cb_, nullptr), ptr, false) {
    other.ptr_ = nullptr;
  }

  shared_ptr(const shared_ptr& other) noexcept
      : shared_ptr(other, other.ptr_) {}

  template <typename Y>
    requires std::is_convertible_v<Y*, T*>
  shared_ptr(const shared_ptr<Y>& other) noexcept
      : shared_ptr(other, static_cast<T*>(other.ptr_)) {}

  shared_ptr(shared_ptr&& other) noexcept
      : shared_ptr(std::move(other), other.ptr_) {}

  template <typename Y>
    requires std::is_convertible_v<Y*, T*>
  shared_ptr(shared_ptr<Y>&& other) noexcept
      : shared_ptr(std::move(other), static_cast<T*>(other.ptr_)) {}

  shared_ptr& operator=(const shared_ptr& other) noexcept {
    shared_ptr temp = other;
    swap(temp, *this);
    return *this;
  }

  template <typename Y>
    requires std::is_convertible_v<Y*, T*>
  shared_ptr& operator=(const shared_ptr<Y>& other) noexcept {
    shared_ptr temp = other;
    swap(temp, *this);
    return *this;
  }

  shared_ptr& operator=(shared_ptr&& other) noexcept {
    shared_ptr temp = std::move(other);
    swap(*this, temp);
    return *this;
  }

  template <typename Y>
    requires std::is_convertible_v<Y*, T*>
  shared_ptr& operator=(shared_ptr<Y>&& other) noexcept {
    shared_ptr temp = std::move(other);
    swap(*this, temp);
    return *this;
  }

  T* get() const noexcept {
    return ptr_;
  }

  operator bool() const noexcept {
    return get() != nullptr;
  }

  T& operator*() const noexcept {
    return *get();
  }

  T* operator->() const noexcept {
    return get();
  }

  std::size_t use_count() const noexcept {
    return (cb_ == nullptr ? 0 : cb_->get_strong_count());
  }

  void reset() noexcept {
    *this = shared_ptr();
  }

  template <typename Y>
    requires std::is_convertible_v<Y*, T*>
  void reset(Y* new_ptr) {
    *this = shared_ptr(new_ptr);
  }

  template <typename Y, typename Deleter = std::default_delete<Y>>
    requires std::is_convertible_v<Y*, T*>
  void reset(Y* new_ptr, Deleter deleter) {
    *this = shared_ptr(new_ptr, std::move(deleter));
  }

  friend bool operator==(const shared_ptr& lhs, const shared_ptr& rhs) noexcept {
    return lhs.get() == rhs.get();
  }

  friend bool operator!=(const shared_ptr& lhs, const shared_ptr& rhs) noexcept {
    return !(lhs == rhs);
  }

  ~shared_ptr() {
    if (cb_) {
      cb_->decrease_strong();
    }
  }

private:
  shared_ptr(control_block* cb, T* ptr, bool is_inc = true) noexcept
      : ptr_(ptr)
      , cb_(cb) {
    if (cb_ && is_inc) {
      cb_->increase_strong();
    }
  }

private:
  T* ptr_;
  control_block* cb_;
};

template <typename T>
class weak_ptr {
public:
  friend void swap(weak_ptr& l, weak_ptr& r) noexcept {
    std::swap(l.cb_, r.cb_);
    std::swap(l.ptr_, r.ptr_);
  }
  template <typename Y>
  friend class weak_ptr;

private:
  weak_ptr(control_block* cb, T* ptr, bool is_inc = true) noexcept
      : cb_(cb)
      , ptr_(ptr) {
    if (cb_ && is_inc) {
      cb_->increase_weak();
    }
  }

public:
  weak_ptr() noexcept
      : cb_(nullptr)
      , ptr_(nullptr) {}

  template <typename Y>
    requires std::is_convertible_v<Y*, T*>
  weak_ptr(const shared_ptr<Y>& other) noexcept
      : weak_ptr(other.cb_, static_cast<T*>(other.ptr_)) {}

  weak_ptr(const weak_ptr& other) noexcept
      : weak_ptr(other.cb_, other.ptr_) {}

  template <typename Y>
    requires std::is_convertible_v<Y*, T*>
  weak_ptr(const weak_ptr<Y>& other) noexcept
      : weak_ptr(other.cb_, static_cast<T*>(other.ptr_)) {}

  weak_ptr(weak_ptr&& other) noexcept
      : weak_ptr(std::exchange(other.cb_, nullptr), std::exchange(other.ptr_, nullptr), false) {}

  template <typename Y>
    requires std::is_convertible_v<Y*, T*>
  weak_ptr(weak_ptr<Y>&& other) noexcept
      : weak_ptr(std::exchange(other.cb_, nullptr), std::exchange(other.ptr_, nullptr), false) {}

  template <typename Y>
    requires std::is_convertible_v<Y*, T*>
  weak_ptr& operator=(const shared_ptr<Y>& other) noexcept {
    weak_ptr temp = other;
    swap(*this, temp);
    return *this;
  }

  weak_ptr& operator=(const weak_ptr& other) noexcept {
    weak_ptr temp = other;
    swap(*this, temp);
    return *this;
  }

  template <typename Y>
    requires std::is_convertible_v<Y*, T*>
  weak_ptr& operator=(const weak_ptr<Y>& other) noexcept {
    weak_ptr temp = other;
    swap(*this, temp);
    return *this;
  }

  weak_ptr& operator=(weak_ptr&& other) noexcept {
    weak_ptr temp = std::move(other);
    swap(*this, temp);
    return *this;
  }

  template <typename Y>
    requires std::is_convertible_v<Y*, T*>
  weak_ptr& operator=(weak_ptr<Y>&& other) noexcept {
    weak_ptr temp = std::move(other);
    swap(*this, temp);
    return *this;
  }

  shared_ptr<T> lock() const noexcept {
    return ((cb_ == nullptr || cb_->get_strong_count() == 0) ? shared_ptr<T>() : shared_ptr<T>(cb_, ptr_));
  }

  void reset() noexcept {
    weak_ptr temp = weak_ptr();
    swap(*this, temp);
  }

  ~weak_ptr() {
    if (cb_) {
      cb_->decrease_weak();
    }
  }

private:
  control_block* cb_;
  T* ptr_;
};

template <typename T, typename... Args>
shared_ptr<T> make_shared(Args&&... args) {
  auto* cb = new control_block_obj<T>(std::forward<Args>(args)...);
  auto res = shared_ptr(cb, cb->get_data(), false);
  return res;
}

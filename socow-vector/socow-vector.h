#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <functional>
#include <memory>

template <typename T, std::size_t SMALL_SIZE>
class socow_vector {
public:
  using value_type = T;
  using reference = T&;
  using const_reference = const T&;
  using pointer = T*;
  using const_pointer = const T*;
  using iterator = pointer;
  using const_iterator = const_pointer;

private:
  struct impl {
  private:
    size_t _capacity;
    size_t _count;

    T _data[0];
    friend socow_vector;

    impl()
        : _count(1) {}
  };

  size_t _size;
  bool _small;

  union {
    impl* _impl;
    T _data[SMALL_SIZE];
  };

  bool is_small() const noexcept {
    return _small;
  }

  static void decrease_impl(impl* cur_impl, size_t sz) noexcept {
    cur_impl->_count--;
    if (cur_impl->_count == 0) {
      std::destroy_n(cur_impl->_data, sz);
      operator delete(cur_impl);
    }
  }

  static impl* get_shared_buff(size_t capacity) {
    impl* res = static_cast<impl*>(operator new(sizeof(impl) + capacity * sizeof(T)));
    res->_count = 1;
    res->_capacity = capacity;
    return res;
  }

  const_pointer get_data() const noexcept {
    return (is_small() ? _data : _impl->_data);
  }

  pointer get_data() noexcept {
    return (is_small() ? _data : _impl->_data);
  }

  void destroy() noexcept {
    if (is_small()) {
      std::destroy_n(_data, size());
    } else {
      decrease_impl(_impl, size());
    }
    _size = 0;
    _small = true;
  }

  // used for shared vector
  void resize(size_t new_capacity) {
    impl* cur_impl = get_shared_buff(new_capacity);
    try {
      std::uninitialized_copy_n(_impl->_data, size(), cur_impl->_data);
    } catch (...) {
      operator delete(cur_impl);
      throw;
    }
    decrease_impl(_impl, size());
    _impl = cur_impl;
  }

  // used for not shared big vector
  void move_resize(size_t new_capacity) {
    impl* new_impl = get_shared_buff(new_capacity);
    std::uninitialized_move_n(_impl->_data, size(), new_impl->_data);
    std::destroy_n(_impl->_data, size());
    operator delete(_impl);
    _impl = new_impl;
  }

  void make_big(size_t new_capacity) {
    impl* new_impl = get_shared_buff(new_capacity);
    std::uninitialized_move_n(_data, size(), new_impl->_data);
    std::destroy_n(_data, size());
    _impl = new_impl;
    _small = false;
  }

  void make_small(size_t newsz) {
    impl* cur_impl = _impl;
    if (cur_impl->_count == 1) {
      std::uninitialized_move_n(cur_impl->_data, newsz, _data);
    } else {
      try {
        std::uninitialized_copy_n(cur_impl->_data, newsz, _data);
      } catch (...) {
        _impl = cur_impl;
        throw;
      }
    }
    decrease_impl(cur_impl, size());
    _small = true;
    _size = newsz;
  }

  template <typename F1 = decltype([]() {})>
  void abstract_push_back(F1 get_reference) {
    if (!is_small() && _impl->_count != 1) {
      resize(size() + 1);
    }
    if (size() == capacity()) {
      socow_vector temp;
      temp.reserve(2 * capacity() + 1);
      new (temp.get_data() + size()) T(get_reference());
      std::uninitialized_move_n(get_data(), size(), temp.get_data());
      temp._size = size() + 1;
      destroy();
      temp.swap(*this);
    } else {
      new (get_data() + size()) T(get_reference());
      ++_size;
    }
  }

public:
  socow_vector() noexcept
      : _size(0)
      , _small(true) {}

  socow_vector(const socow_vector& other) {
    _size = other.size();
    _small = other._small;
    if (other.is_small()) {
      std::uninitialized_copy_n(other._data, size(), _data);
    } else {
      _impl = other._impl;
      _impl->_count++;
    }
  }

  socow_vector(socow_vector&& other) noexcept
      : socow_vector() {
    (*this) = std::move(other);
  }

  socow_vector& operator=(const socow_vector& other) {
    if (get_data() == other.get_data()) {
      return *this;
    }
    socow_vector temp(other);
    destroy();
    temp.swap(*this);
    return *this;
  }

  socow_vector& operator=(socow_vector&& other) noexcept {
    if (this == &other) {
      return *this;
    }
    destroy();
    if (other.is_small()) {
      std::uninitialized_move_n(other._data, other.size(), _data);
      std::destroy_n(other._data, other.size());
    } else {
      _impl = other._impl;
    }
    _size = other.size();
    _small = other._small;
    other._size = 0;
    other._small = true;
    return *this;
  }

  ~socow_vector() noexcept {
    destroy();
  }

  reference operator[](size_t index) {
    return data()[index];
  }

  const_reference operator[](size_t index) const noexcept {
    return data()[index];
  }

  pointer data() {
    if (!is_small() && _impl->_count != 1) {
      if (size() <= SMALL_SIZE) {
        make_small(size());
      } else {
        resize(size());
      }
    }
    return get_data();
  }

  const_pointer data() const noexcept {
    return get_data();
  }

  size_t size() const noexcept {
    return _size;
  }

  reference front() {
    return data()[0];
  }

  const_reference front() const noexcept {
    return data()[0];
  }

  reference back() {
    return data()[size() - 1];
  }

  const_reference back() const noexcept {
    return data()[size() - 1];
  }

  void push_back(T&& value) {
    abstract_push_back([&value]() mutable -> T&& { return std::move(value); });
  }

  void push_back(const T& value) {
    abstract_push_back([&value]() mutable -> const T& { return value; });
  }

  void pop_back() {
    erase(get_data() + size() - 1);
  }

  bool empty() const noexcept {
    return size() == 0;
  }

  size_t capacity() const noexcept {
    return (is_small() ? SMALL_SIZE : _impl->_capacity);
  }

  void reserve(size_t new_capacity) {
    if (is_small() && new_capacity > capacity()) {
      make_big(new_capacity);
    } else if (!is_small()) {
      if (_impl->_count != 1 && new_capacity > size()) {
        if (new_capacity <= SMALL_SIZE) {
          make_small(size());
        } else {
          resize(new_capacity);
        }
      } else if (_impl->_count == 1 && new_capacity > capacity()) {
        move_resize(new_capacity);
      }
    }
  }

  void shrink_to_fit() {
    if (!is_small() && size() != capacity()) {
      if (size() > SMALL_SIZE) {
        if (_impl->_count != 1) {
          resize(size());
        } else {
          move_resize(size());
        }
      } else {
        make_small(size());
      }
    }
  }

  void clear() noexcept {
    if (empty()) {
      return;
    }
    if (is_small() || _impl->_count == 1) {
      std::destroy_n(get_data(), size());
      _size = 0;
    } else {
      destroy();
    }
  }

  void swap(socow_vector& other) noexcept {
    if (get_data() == other.get_data()) {
      return;
    }
    if ((other.is_small() && !is_small()) || (other.is_small() && is_small() && size() < other.size())) {
      other.swap(*this);
      return;
    }
    socow_vector tmp(std::move(other));
    other = std::move(*this);
    (*this) = std::move(tmp);
  }

  iterator begin() {
    return data();
  }

  iterator end() {
    return begin() + size();
  }

  const_iterator begin() const noexcept {
    return data();
  }

  const_iterator end() const noexcept {
    return begin() + size();
  }

  iterator insert(const_iterator it, const T& value) {
    size_t pos = it - get_data();
    push_back(value);
    std::rotate(get_data() + pos, (get_data() + size()) - 1, get_data() + size());
    return data() + pos;
  }

  iterator insert(const_iterator it, T&& value) {
    size_t pos = it - get_data();
    push_back(std::move(value));
    std::rotate(get_data() + pos, (get_data() + size()) - 1, get_data() + size());
    return data() + pos;
  }

  iterator erase(const_iterator it) {
    return erase(it, it + 1);
  }

  iterator erase(const_iterator first, const_iterator last) {
    if (first == last || first == get_data() + size()) {
      size_t dist = (first - get_data());
      return begin() + dist;
    }
    size_t posl = first - get_data(), posr = last - get_data();
    if (is_small() || _impl->_count == 1) {
      std::rotate(begin() + posl, begin() + posr, end());
      std::destroy(end() + posl - posr, end());
      _size -= posr - posl;
    } else {
      socow_vector tmp;
      tmp.reserve(size() + posl - posr);
      for (size_t i = 0; i < posl; ++i) {
        tmp.push_back(get_data()[i]);
      }
      for (size_t i = posr; i < size(); ++i) {
        tmp.push_back(get_data()[i]);
      }
      tmp.swap(*this);
    }
    return get_data() + posl;
  }
};

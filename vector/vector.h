#pragma once

#include <cstddef>
#include <memory>

template <typename T>
class vector {
public:
  using value_type = T;
  using reference = T&;
  using const_reference = const T&;
  using pointer = T*;
  using const_pointer = const T*;
  using iterator = pointer;
  using const_iterator = const_pointer;

private:
  pointer _data;
  size_t _size;
  size_t _capacity;

  static void destroy(pointer p, size_t count) {
    for (size_t i = 0; i < count; ++i) {
      p[count - i - 1].~T();
    }
  }

  static void copy_from(pointer source, pointer dest, size_t count) {
    for (size_t i = 0; i < count; ++i) {
      try {
        new (dest + i) T(source[i]);
      } catch (...) {
        destroy(dest, i);
        operator delete(dest);
        throw;
      }
    }
  }

  void resize(size_t newcapacity) {
    if (newcapacity == 0) {
      operator delete(data());
      _data = nullptr;
      _capacity = 0;
      return;
    }
    pointer newdata = static_cast<pointer>(operator new(newcapacity * sizeof(T)));
    copy_from(_data, newdata, size());
    destroy(data(), size());
    operator delete(data());
    _capacity = newcapacity;
    _data = newdata;
  }

public:
  vector() noexcept
      : _data(nullptr)
      , _size(0)
      , _capacity(0) {}

  vector(const vector& other) {
    _capacity = other._size;
    _size = other._size;
    _data = nullptr;
    if (size() != 0) {
      _data = static_cast<pointer>(operator new(_capacity * sizeof(T)));
      copy_from(other._data, _data, size());
    }
  }

  vector(vector&& other) {
    _size = other._size;
    _capacity = other._capacity;
    _data = other.data();
    other._data = nullptr;
    other._size = 0;
    other._capacity = 0;
  }

  vector& operator=(const vector& other) {
    if (data() == other.data()) {
      return *this;
    }
    vector vec = other;
    vec.swap(*this);
    return *this;
  }

  vector& operator=(vector&& other) {
    other.swap(*this);
    return *this;
  }

  ~vector() noexcept {
    destroy(data(), size());
    operator delete(data());
  }

  reference operator[](size_t index) {
    return data()[index];
  }

  const_reference operator[](size_t index) const {
    return data()[index];
  }

  pointer data() noexcept {
    return _data;
  }

  const_pointer data() const noexcept {
    return _data;
  }

  size_t size() const noexcept {
    return _size;
  }

  reference front() {
    return data()[0];
  }

  const_reference front() const {
    return data()[0];
  }

  reference back() {
    return data()[size() - 1];
  }

  const_reference back() const {
    return data()[size() - 1];
  }

  void push_back(T value) {
    if (size() == capacity()) {
      resize(2 * capacity() + 1);
    }
    new (data() + size()) T(std::move(value));
    _size++;
  }

  void pop_back() {
    data()[size() - 1].~T();
    _size--;
  }

  bool empty() const noexcept {
    return size() == 0;
  }

  size_t capacity() const noexcept {
    return _capacity;
  }

  void reserve(size_t new_capacity) {
    if (new_capacity > capacity()) {
      resize(new_capacity);
    }
  }

  void shrink_to_fit() {
    if (capacity() > size()) {
      resize(size());
    }
  }

  void clear() {
    for (size_t i = 0; i < size(); ++i) {
      data()[size() - i - 1].~T();
    }
    _size = 0;
  }

  void swap(vector& other) noexcept {
    std::swap(_data, other._data);
    std::swap(_size, other._size);
    std::swap(_capacity, other._capacity);
  }

  iterator begin() noexcept {
    return data();
  }

  iterator end() noexcept {
    return begin() + size();
  }

  const_iterator begin() const noexcept {
    return data();
  }

  const_iterator end() const noexcept {
    return begin() + size();
  }

  iterator insert(const_iterator pos, const T& value) {
    size_t numpos = pos - begin();
    push_back(value);
    for (size_t i = 0; i < size() - numpos - 1; ++i) {
      std::swap(data()[size() - i - 1], data()[size() - i - 2]);
    }
    return data() + numpos;
  }

  iterator erase(const_iterator pos) {
    return erase(pos, pos + 1);
  }

  iterator erase(const_iterator first, const_iterator last) {
    size_t posl = first - begin(), posr = last - begin();
    for (size_t i = 0; posr + i < size(); ++i) {
      std::swap(data()[posl + i], data()[posr + i]);
    }
    for (size_t i = 0; posl + i < posr; ++i) {
      pop_back();
    }
    return data() + posl;
  }
};

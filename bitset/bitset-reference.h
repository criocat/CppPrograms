#pragma once

#include "bitset-classes.h"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <ostream>
#include <string>
using namespace bitset_members;

template <typename T>
struct bit {
public:
  using value_type = bool;

private:
  size_t _pos;
  T _data;
  static const size_t all_ones = (~static_cast<size_t>(0));

  friend base_iterator<bit<word_pointer>>;
  friend base_iterator<bit<const word_pointer>>;

  friend base_view<bit<word_pointer>>;
  friend base_view<bit<const word_pointer>>;

  friend bit<word_pointer>;

  friend bitset;

  friend bitset operator<<(const base_view<bit<const word_pointer>>& vi, size_t count);
  friend bitset operator>>(const base_view<bit<const word_pointer>>& bs, size_t count);
  friend bool
  operator==(const base_view<bit<const word_pointer>>& left, const base_view<bit<const word_pointer>>& right);

  bit(T data, size_t pos)
      : _pos(pos)
      , _data(data) {}

  base_word_type get_word(size_t len) const {
    base_word_type res = (*_data << (word_len - _pos - 1));
    if (len >= _pos + 2) {
      res |= ((*(_data + 1) >> (_pos + 1)) & (~(all_ones << (word_len - _pos - 1))));
    }
    return (res & (all_ones << (word_len - len)));
  }

  base_word_type get_mask(size_t pos, size_t len) const {
    return ((pos == 0 ? 0 : (all_ones << (word_len - pos))) ^ (all_ones << (word_len - (pos + len))));
  }

  void change_word(size_t len, base_word_type word) const {
    base_word_type mask = get_mask(word_len - _pos - 1, std::min(len, _pos + 1));
    (*_data) = (((*_data) & (~mask)) | ((word >> (word_len - _pos - 1)) & mask));
    if (len >= _pos + 2) {
      mask = get_mask(0, len - _pos - 1);
      *(_data + 1) = ((*(_data + 1) & (~mask)) | ((word << (_pos + 1)) & mask));
    }
  }

public:
  bit() = delete;
  bit(const bit& other) = default;
  ~bit() = default;

  operator bool() const {
    return (*_data & (static_cast<base_word_type>(1) << _pos)) != static_cast<base_word_type>(0);
  }

  operator bit<const T>() const {
    return {_data, _pos};
  }

  bit operator=(bool val) const {
    if (val) {
      *_data = ((*_data) | (static_cast<base_word_type>(1) << _pos));
    } else {
      *_data = ((*_data) & (~(static_cast<base_word_type>(1) << _pos)));
    }
    return *this;
  }

  void flip() const {
    *_data = ((*_data) ^ (static_cast<base_word_type>(1) << _pos));
  }
};

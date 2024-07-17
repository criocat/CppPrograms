#pragma once

#include "bitset-reference.h"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <ostream>
#include <string>
using namespace bitset_members;

template <typename G>
struct base_iterator {
public:
  using difference_type = std::ptrdiff_t;
  using value_type = bool;
  using pointer = void;
  using reference = G;
  using iterator_category = std::random_access_iterator_tag;
  using word_type = base_word_type;

private:
  word_pointer _data;
  size_t _pos;
  friend bitset;

  friend base_view<bit<word_pointer>>;
  friend base_view<bit<const word_pointer>>;

public:
  base_iterator() = default;

  base_iterator(word_pointer data, size_t pos)
      : _data(data)
      , _pos(pos) {}

  operator base_iterator<bit<const word_pointer>>() const {
    return {_data, _pos};
  }

  reference operator*() const {
    return reference(_data, _pos);
  }

  base_iterator& operator++() {
    if (_pos == 0) {
      _pos = word_len - 1;
      _data++;
    } else {
      _pos--;
    }
    return *this;
  }

  base_iterator operator++(int) {
    base_iterator res = base_iterator(_data, _pos);
    ++(*this);
    return res;
  }

  base_iterator& operator--() {
    if (_pos == word_len - 1) {
      _pos = 0;
      _data--;
    } else {
      _pos++;
    }
    return *this;
  }

  base_iterator operator--(int) {
    base_iterator res = base_iterator(_data, _pos);
    --(*this);
    return res;
  }

  friend base_iterator operator+(const difference_type left, const base_iterator& right) {
    base_iterator res = right;
    return res += left;
  }

  friend base_iterator operator+(const base_iterator& left, const difference_type right) {
    return right + left;
  }

  friend base_iterator operator-(const base_iterator& left, const difference_type right) {
    return left + (right * -1);
  }

  friend difference_type operator-(const base_iterator& left, const base_iterator& right) {
    difference_type word_dist = left._data - right._data;
    return (word_dist * static_cast<difference_type>(word_len) + static_cast<difference_type>(right._pos)) -
           static_cast<difference_type>(left._pos);
  }

  friend bool operator<(const base_iterator& left, const base_iterator& right) {
    return right - left > 0;
  }

  friend bool operator>(const base_iterator& left, const base_iterator& right) {
    return left - right > 0;
  }

  friend bool operator>=(const base_iterator& left, const base_iterator& right) {
    return !(left < right);
  }

  friend bool operator<=(const base_iterator& left, const base_iterator& right) {
    return !(left > right);
  }

  base_iterator& operator+=(const difference_type numdiff) {
    difference_type diff = static_cast<difference_type>((word_len - _pos) - 1) + numdiff;
    difference_type word_diff =
        (diff - static_cast<difference_type>(diff >= 0 ? 0 : word_len - 1)) / static_cast<difference_type>(word_len);
    _pos = word_len - (((std::abs(word_diff) + 1) * static_cast<difference_type>(word_len) + diff) % word_len) - 1;
    _data += word_diff;
    return *this;
  }

  base_iterator& operator-=(const difference_type diff) {
    return (*this += -diff);
  }

  reference operator[](const difference_type pos) const {
    return *(*this + pos);
  }

  friend bool operator==(const base_iterator& left, const base_iterator& right) {
    return left._data == right._data && left._pos == right._pos;
  }

  friend bool operator!=(const base_iterator& left, const base_iterator& right) {
    return !(left == right);
  }
};

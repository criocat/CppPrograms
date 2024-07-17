#pragma once

#include "bitset-iterator.h"
#include "bitset-reference.h"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstring>
#include <ostream>
#include <string>

using namespace bitset_members;

template <typename G>
struct base_view {
public:
  using word_type = base_word_type;
  using value_type = bool;
  using reference = G;
  using const_reference = bit<const word_pointer>;
  using iterator = base_iterator<reference>;
  using const_iterator = base_iterator<const_reference>;
  using view = base_view<reference>;
  using const_view = base_view<const_reference>;

private:
  iterator _it_st, _it_end;

  friend bitset;
  friend base_view<bit<word_pointer>>;

  base_view(const iterator it_st, const iterator it_end)
      : _it_st(it_st)
      , _it_end(it_end) {}

  base_view base_change_opearator(const const_view& other, word_type func(word_type w1, word_type w2)) const {
    for (size_t i = 0; i < size(); i += word_len) {
      size_t curlen = std::min(size() - i, word_len);
      (*this)[i].change_word(curlen, func((*this)[i].get_word(curlen), other[i].get_word(curlen)));
    }
    return *this;
  }

public:
  static constexpr size_t npos = (~static_cast<size_t>(0));

  base_view() = default;

  base_view(const base_view& other) = default;

  base_view& operator=(const base_view& other) & = default;

  operator const_view() const {
    return const_view(begin(), end());
  }

  iterator begin() const {
    return _it_st;
  }

  iterator end() const {
    return _it_end;
  }

  size_t size() const {
    return end() - begin();
  }

  reference operator[](size_t pos) const {
    return *(begin() + pos);
  }

  friend bool
  operator==(const base_view<bit<const word_pointer>>& left, const base_view<bit<const word_pointer>>& right);

  friend bool
  operator!=(const base_view<bit<const word_pointer>>& left, const base_view<bit<const word_pointer>>& right);

  bool empty() const {
    return size() == 0;
  }

  base_view operator&=(const const_view& other) const {
    return base_change_opearator(other, [](word_type w1, word_type w2) { return w1 & w2; });
  }

  base_view operator|=(const const_view& other) const {
    return base_change_opearator(other, [](word_type w1, word_type w2) { return w1 | w2; });
  }

  base_view operator^=(const const_view& other) const {
    return base_change_opearator(other, [](word_type w1, word_type w2) { return w1 ^ w2; });
  }

  friend bitset operator<<(const const_view& vi, size_t count);

  friend bitset operator>>(const const_view& bs, size_t count);

  base_view flip() const {
    return base_change_opearator(*this, [](word_type w1, word_type) { return w1 ^ (~static_cast<word_type>(0)); });
  }

  base_view set() const {
    return base_change_opearator(*this, [](word_type, word_type) -> word_type { return ~static_cast<word_type>(0); });
  }

  base_view reset() const {
    return base_change_opearator(*this, [](word_type, word_type) -> word_type { return static_cast<word_type>(0); });
  }

  bool all() const {
    for (size_t i = 0; i < size(); i += word_len) {
      size_t curlen = std::min(size() - i, word_len);
      if ((*this)[i].get_word(curlen) != ((~static_cast<word_type>(0)) << (word_len - curlen))) {
        return false;
      }
    }
    return true;
  }

  bool any() const {
    for (size_t i = 0; i < size(); i += word_len) {
      size_t curlen = std::min(size() - i, word_len);
      if ((*this)[i].get_word(curlen) != 0) {
        return true;
      }
    }
    return false;
  }

  size_t count() const {
    size_t res = 0;
    for (size_t i = 0; i < size(); i += word_len) {
      size_t curlen = std::min(size() - i, word_len);
      res += std::popcount((*this)[i].get_word(curlen));
    }
    return res;
  }

  view subview(size_t offset = 0, size_t count = npos) const {
    size_t pos1 = std::min(offset, size());
    size_t pos2 = std::min(size() - pos1, count);
    return {begin() + pos1, begin() + pos1 + pos2};
  }

  void swap(base_view& other) {
    std::swap(other._it_st, _it_st);
    std::swap(other._it_end, _it_end);
  }

  friend void swap(base_view& lhs, base_view& rhs) {
    lhs.swap(rhs);
  }
};

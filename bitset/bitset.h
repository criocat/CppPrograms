#pragma once

#include "bitset-iterator.h"
#include "bitset-reference.h"
#include "bitset-view.h"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <ostream>
#include <string>

using namespace bitset_members;

struct bitset {
public:
  using value_type = bool;
  using word_type = base_word_type;
  using reference = bit<word_pointer>;
  using const_reference = bit<const word_pointer>;
  using iterator = base_iterator<reference>;
  using const_iterator = base_iterator<const_reference>;
  using view = base_view<reference>;
  using const_view = base_view<const_reference>;

private:
  word_pointer _data;
  size_t _size;
  size_t _word_count;

  size_t get_word_pos(size_t pos) const;
  size_t get_pos_in_word(size_t pos) const;
  bitset(size_t word_size);

public:
  static constexpr size_t npos = (~static_cast<size_t>(0));
  bitset();
  bitset(size_t size, bool value);
  bitset(const_iterator first, const_iterator last);
  explicit bitset(const const_view& other);
  bitset(const bitset& other);
  explicit bitset(std::string_view str);
  bitset& operator=(const bitset& other) &;
  bitset& operator=(std::string_view str) &;
  bitset& operator=(const const_view& other) &;
  ~bitset();
  void swap(bitset& other);
  reference operator[](size_t pos);
  const_reference operator[](size_t pos) const;
  iterator begin();
  const_iterator begin() const;
  iterator end();
  const_iterator end() const;
  size_t size() const;
  bool empty() const;
  bitset& operator&=(const const_view& other) &;
  bitset& operator|=(const const_view& other) &;
  bitset& operator^=(const const_view& other) &;
  bitset& operator>>=(size_t count) &;
  bitset& operator<<=(size_t count) &;
  void flip() &;
  bitset& set() &;
  bitset& reset() &;
  bool all() const;
  bool any() const;
  size_t count() const;
  operator view();
  operator const_view() const;
  view subview(size_t offset = 0, size_t count = npos);
  const_view subview(size_t offset = 0, size_t count = npos) const;
  friend void swap(bitset& lhs, bitset& rhs);
};

bitset operator&(const bitset::const_view& lhs, const bitset::const_view& rhs);
bitset operator|(const bitset::const_view& lhs, const bitset::const_view& rhs);
bitset operator^(const bitset::const_view& lhs, const bitset::const_view& rhs);
bitset operator~(const bitset::const_view& rhs);
bitset operator<<(const bitset::const_view& vi, size_t count);
bitset operator>>(const bitset::const_view& vi, size_t count);
std::string to_string(const bitset::const_view& bs);
std::ostream& operator<<(std::ostream& out, const bitset::const_view& bs);
bool operator==(const bitset::const_view& left, const bitset::const_view& right);
bool operator!=(const bitset::const_view& left, const bitset::const_view& right);

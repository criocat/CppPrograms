#include "bitset.h"

#include "bitset-iterator.h"
#include "bitset-reference.h"
#include "bitset-view.h"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <ostream>
#include <string>

size_t bitset::get_word_pos(size_t pos) const {
  return (pos + word_len - 1) / word_len;
}

size_t bitset::get_pos_in_word(size_t pos) const {
  return (word_len - (pos % word_len)) - 1;
}

bitset::bitset()
    : _data(nullptr)
    , _size(0)
    , _word_count(0) {}

bitset::bitset(size_t size)
    : _size(size)
    , _word_count(get_word_pos(size)) {
  _data = new word_type[_word_count]();
}

bitset::bitset(size_t size, bool value)
    : bitset(size) {
  for (size_t i = 0; i < _word_count; ++i) {
    _data[i] = (value ? (~static_cast<word_type>(0)) : 0);
  }
}

bitset::bitset(const_iterator first, const_iterator last)
    : bitset(const_view(first, last)) {}

bitset::bitset(const const_view& other)
    : bitset(other.size()) {
  for (size_t i = 0; i * word_len < other.size(); ++i) {
    size_t curlen = std::min(other.size() - i * word_len, word_len);
    _data[i] = other[i * word_len].get_word(curlen);
  }
}

bitset::bitset(const bitset& other)
    : bitset(other.size()) {
  std::copy_n(other._data, other._word_count, _data);
}

bitset::bitset(std::string_view str)
    : bitset(str.size()) {
  for (size_t i = 0; i < str.size(); ++i) {
    (*this)[i] = (str[i] == '1');
  }
}

bitset& bitset::operator=(const bitset& other) & {
  if (_data == other._data) {
    return *this;
  }
  bitset temp(other);
  temp.swap(*this);
  return *this;
}

bitset& bitset::operator=(std::string_view str) & {
  bitset temp(str);
  temp.swap(*this);
  return *this;
}

bitset& bitset::operator=(const const_view& other) & {
  bitset b(other);
  b.swap(*this);
  return *this;
}

bitset::~bitset() {
  delete[] (_data);
}

void bitset::swap(bitset& other) {
  std::swap(_size, other._size);
  std::swap(_word_count, other._word_count);
  std::swap(_data, other._data);
}

bitset::reference bitset::operator[](size_t pos) {
  return {_data + (pos / word_len), get_pos_in_word(pos)};
}

bitset::const_reference bitset::operator[](size_t pos) const {
  return {_data + (pos / word_len), get_pos_in_word(pos)};
}

bitset::iterator bitset::begin() {
  return {_data, word_len - 1};
}

bitset::const_iterator bitset::begin() const {
  return {_data, word_len - 1};
}

bitset::iterator bitset::end() {
  return begin() + _size;
}

bitset::const_iterator bitset::end() const {
  return begin() + _size;
}

size_t bitset::size() const {
  return _size;
}

bool bitset::empty() const {
  return _size == 0;
}

bitset& bitset::operator&=(const const_view& other) & {
  (*this).subview() &= other.subview();
  return *this;
}

bitset& bitset::operator|=(const const_view& other) & {
  (*this).subview() |= other.subview();
  return *this;
}

bitset& bitset::operator^=(const const_view& other) & {
  (*this).subview() ^= other.subview();
  return *this;
}

bitset& bitset::operator>>=(size_t count) & {
  bitset temp = (*this).subview() >> count;
  temp.swap(*this);
  return *this;
}

bitset& bitset::operator<<=(size_t count) & {
  bitset temp = (*this).subview() << count;
  temp.swap(*this);
  return *this;
}

void bitset::flip() & {
  (*this).subview().flip();
}

bitset& bitset::set() & {
  (*this).subview().set();
  return *this;
}

bitset& bitset::reset() & {
  (*this).subview().reset();
  return *this;
}

bool bitset::all() const {
  return (*this).subview().all();
}

bool bitset::any() const {
  return (*this).subview().any();
}

size_t bitset::count() const {
  return (*this).subview().count();
}

bitset::operator view() {
  return view(begin(), end());
}

bitset::operator const_view() const {
  return const_view(begin(), end());
}

bitset::view bitset::subview(size_t offset, size_t count) {
  size_t pos1 = std::min(offset, _size);
  size_t pos2 = std::min(_size - pos1, count);
  return {begin() + pos1, begin() + pos1 + pos2};
}

bitset::const_view bitset::subview(size_t offset, size_t count) const {
  size_t pos1 = std::min(offset, _size);
  size_t pos2 = std::min(_size - pos1, count);
  return {begin() + pos1, begin() + pos1 + pos2};
}

void swap(bitset& lhs, bitset& rhs) {
  lhs.swap(rhs);
}

bitset operator&(const bitset::const_view& lhs, const bitset::const_view& rhs) {
  bitset res(lhs);
  res.subview() &= rhs;
  return res;
}

bitset operator|(const bitset::const_view& lhs, const bitset::const_view& rhs) {
  bitset res(lhs);
  res.subview() |= rhs;
  return res;
}

bitset operator^(const bitset::const_view& lhs, const bitset::const_view& rhs) {
  bitset res(lhs);
  res.subview() ^= rhs;
  return res;
}

bitset operator~(const bitset::const_view& rhs) {
  bitset res(rhs);
  res.subview().flip();
  return res;
}

bitset operator<<(const bitset::const_view& vi, size_t count) {
  bitset res = bitset(vi.size() + count, false);
  for (size_t i = 0; i < vi.size(); i += static_cast<size_t>(16)) {
    size_t curlen = std::min(vi.size() - i, static_cast<size_t>(16));
    res[i].change_word(curlen, vi[i].get_word(curlen));
  }
  return res;
}

bitset operator>>(const bitset::const_view& vi, size_t count) {
  count = std::min(count, vi.size());
  bitset res(vi.size() - count, false);
  for (size_t i = 0; i < res.size(); i += word_len) {
    size_t curlen = std::min(res.size() - i, word_len);
    res[i].change_word(curlen, vi[i].get_word(curlen));
  }
  return res;
}

std::string to_string(const bitset::const_view& bs) {
  std::string str;
  str.reserve(bs.size());
  for (bool b : bs) {
    str.push_back(b ? '1' : '0');
  }
  return str;
}

std::ostream& operator<<(std::ostream& out, const bitset::const_view& bs) {
  for (bool b : bs) {
    out << b;
  }
  return out;
}

bool operator==(const bitset::const_view& left, const bitset::const_view& right) {
  if (left.size() != right.size()) {
    return false;
  }
  for (size_t i = 0; i < left.size(); i += word_len) {
    size_t curlen = std::min(left.size() - i, word_len);
    if (left[i].get_word(curlen) != right[i].get_word(curlen)) {
      return false;
    }
  }
  return true;
}

bool operator!=(const bitset::const_view& left, const bitset::const_view& right) {
  return !(left == right);
}

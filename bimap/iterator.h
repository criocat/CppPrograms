#pragma once

#include "nodes.h"

#include <iterator>
#include <type_traits>

template <typename T, typename Other, typename CompareLeft, typename CompareRight>
class bimap;

namespace bimap_components {

template <typename T, typename Other, typename Compare, typename Tag>
class map;

template <typename T, typename Other, typename Tag>
class base_iterator {
  using Left = std::conditional_t<std::is_same_v<Tag, tag_left>, T, Other>;
  using Right = std::conditional_t<std::is_same_v<Tag, tag_left>, Other, T>;
  using OtherTag = std::conditional_t<std::is_same_v<Tag, tag_left>, tag_right, tag_left>;
  using OtherIterator = base_iterator<Other, T, OtherTag>;

public:
  using difference_type = std::ptrdiff_t;
  using value_type = T;
  using pointer = const T*;
  using reference = const T&;
  using iterator_category = std::bidirectional_iterator_tag;

private:
  template <typename Y, typename OtherY, typename C, typename Tg>
  friend class map;

  template <typename Y, typename OtherY, typename CL, typename CR>
  friend class ::bimap;

  template <typename Y, typename OtherY, typename Tg>
  friend class base_iterator;

  base_iterator(base_node<Tag>* cur_node) noexcept
      : nd_(cur_node) {}

  reference as_val(base_node<Tag>* nd) const noexcept {
    return static_cast<value_wrapper<T, Tag>*>(static_cast<value_node<Left, Right>*>(nd))->val_;
  }

public:
  base_iterator() noexcept
      : nd_(nullptr) {}

  reference operator*() const noexcept {
    return as_val(nd_);
  }

  operator OtherIterator() const noexcept {
    return {static_cast<bimap_node*>(nd_)};
  }

  pointer operator->() const noexcept {
    return &as_val(nd_);
  }

  base_iterator& operator++() noexcept {
    nd_ = nd_->get_next();
    return *this;
  }

  base_iterator operator++(int) noexcept {
    base_iterator tmp = *this;
    ++*this;
    return tmp;
  }

  base_iterator& operator--() noexcept {
    nd_ = nd_->get_prev();
    return *this;
  }

  base_iterator operator--(int) noexcept {
    base_iterator tmp = *this;
    --*this;
    return tmp;
  }

  OtherIterator flip() const noexcept {
    return {static_cast<base_node<OtherTag>*>(static_cast<bimap_node*>(nd_))};
  }

  friend bool operator==(const base_iterator& lhs, const base_iterator& rhs) noexcept {
    return lhs.nd_ == rhs.nd_;
  }

  friend bool operator!=(const base_iterator& lhs, const base_iterator& rhs) noexcept {
    return !(lhs == rhs);
  }

private:
  base_node<Tag>* nd_;
};

} // namespace bimap_components

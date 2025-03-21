#pragma once

#include <cstddef>
#include <utility>

namespace bimap_components {

struct tag_left;
struct tag_right;

template <typename Tag>
class base_node {
public:
  base_node* l_;
  base_node* r_;
  base_node* p_;

public:
  base_node() noexcept
      : l_(nullptr)
      , r_(nullptr)
      , p_(nullptr) {}

  static base_node* get_max(base_node* nd) noexcept {
    while (nd->r_ != nullptr) {
      nd = nd->r_;
    }
    return nd;
  }

  static base_node* get_min(base_node* nd) noexcept {
    while (nd->l_ != nullptr) {
      nd = nd->l_;
    }
    return nd;
  }

  base_node* get_next() noexcept {
    base_node* nd = this;
    if (nd->r_ != nullptr) {
      return get_min(nd->r_);
    }
    while (nd->p_->p_ != nd && nd->p_->l_ != nd) {
      nd = nd->p_;
    }
    return nd->p_;
  }

  base_node* get_prev() noexcept {
    base_node* nd = this;
    if (nd->l_ != nullptr) {
      return get_max(nd->l_);
    }
    while (nd->p_->r_ != nd) {
      nd = nd->p_;
    }
    return nd->p_;
  }
};

template <typename T, typename Tag>
class value_wrapper {
public:
  template <typename Val>
  explicit value_wrapper(Val&& val)
      : val_(std::forward<Val>(val)) {}

public:
  T val_;
};

class bimap_node
    : public base_node<tag_left>
    , public base_node<tag_right> {
public:
  bimap_node(bool is_sentinel = false) noexcept {
    if (is_sentinel) {
      base_node<tag_left>::r_ = this;
      base_node<tag_left>::l_ = this;
      base_node<tag_right>::r_ = this;
      base_node<tag_right>::l_ = this;
    }
  }
};

template <typename Left, typename Right>
class value_node
    : public bimap_node
    , public value_wrapper<Left, tag_left>
    , public value_wrapper<Right, tag_right> {
public:
  template <typename L, typename R>
  value_node(L&& left, R&& right)
      : value_wrapper<Left, tag_left>(std::forward<L>(left))
      , value_wrapper<Right, tag_right>(std::forward<R>(right)) {}
};

} // namespace bimap_components

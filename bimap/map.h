#pragma once

#include "iterator.h"

#include <stdexcept>

namespace bimap_components {

template <typename Left, typename Right, typename Compare, typename Tag>
class map {
  using node = base_node<Tag>;
  using T = std::conditional_t<std::is_same_v<Tag, tag_left>, Left, Right>;
  using Other = std::conditional_t<std::is_same_v<Tag, tag_left>, Right, Left>;

public:
  using iterator = base_iterator<T, Other, Tag>;

  template <typename L, typename R, typename C, typename Tg>
  friend class map;

public:
  explicit map(node* sentinel, Compare comp)
      : sentinel_(sentinel)
      , comp_(std::move(comp)) {}

private:
  const T& as_val(node* nd) const noexcept {
    return static_cast<value_wrapper<T, Tag>*>(static_cast<value_node<Left, Right>*>(nd))->val_;
  }

  node* get_root() const noexcept {
    return sentinel_->p_;
  }

  node* get_min() const noexcept {
    return sentinel_->r_;
  }

  node* get_max() const noexcept {
    return sentinel_->l_;
  }

  node*& get_parent_ptr(node* nd) const noexcept {
    if (nd == get_root()) {
      return sentinel_->p_;
    }
    return (nd->p_->l_ == nd ? nd->p_->l_ : nd->p_->r_);
  }

  void unlink(node* nd) noexcept {
    if (nd == get_max()) {
      sentinel_->l_ = nd->get_prev();
    }
    if (nd == get_min()) {
      sentinel_->r_ = nd->get_next();
    }
    if (nd->r_ == nullptr && nd->l_ == nullptr) {
      get_parent_ptr(nd) = nullptr;
    } else if (nd->r_ == nullptr) {
      get_parent_ptr(nd) = nd->l_;
      nd->l_->p_ = nd->p_;
    } else {
      get_parent_ptr(nd) = nd->r_;
      nd->r_->p_ = nd->p_;
    }
  }

public:
  void move_node(node* nd, node* dist) const noexcept {
    dist->l_ = nd->l_;
    dist->r_ = nd->r_;
    dist->p_ = nd->p_;
    if (dist->l_ != nullptr) {
      dist->l_->p_ = dist;
    }
    if (dist->r_ != nullptr) {
      dist->r_->p_ = dist;
    }
    if (dist->p_->l_ == nd) {
      dist->p_->l_ = dist;
    }
    if (dist->p_->r_ == nd) {
      dist->p_->r_ = dist;
    }
    if (sentinel_->r_ == nd) {
      sentinel_->r_ = dist;
    }
    if (sentinel_->l_ == nd) {
      sentinel_->l_ = dist;
    }
    if (sentinel_->p_ == nd) {
      sentinel_->p_ = dist;
    }
  }

  iterator erase(node* nd) noexcept {
    node* res = nd->get_next();
    if (nd->r_ == nullptr || nd->l_ == nullptr) {
      unlink(nd);
    } else {
      unlink(res);
      res->p_ = nd->p_;
      res->r_ = nd->r_;
      res->l_ = nd->l_;
      get_parent_ptr(nd) = res;
      if (res->l_ != nullptr) {
        res->l_->p_ = res;
      }
      if (res->r_ != nullptr) {
        res->r_->p_ = res;
      }
      if (sentinel_->l_ == nd) {
        sentinel_->l_ = res;
      }
      if (sentinel_->r_ == nd) {
        sentinel_->r_ = res;
      }
    }
    return iterator(res);
  }

  iterator begin() const noexcept {
    return iterator(sentinel_->r_);
  }

  iterator end() const noexcept {
    return iterator(sentinel_);
  }

  iterator lower_bound(const T& key) const {
    node* prev = sentinel_;
    node* nd = get_root();
    while (nd != nullptr) {
      if (comp_(key, as_val(nd))) {
        prev = nd;
        nd = nd->l_;
      } else if (comp_(as_val(nd), key)) {
        nd = nd->r_;
      } else {
        return iterator(nd);
      }
    }
    if (prev != sentinel_ && prev == get_max() && comp_(as_val(prev), key)) {
      return end();
    }
    return iterator(prev);
  }

  iterator upper_bound(const T& value) const {
    iterator it = lower_bound(value);
    if (it != end() && !comp_(value, *it)) {
      it++;
    }
    return it;
  }

  iterator find(const T& value) const {
    iterator it = lower_bound(value);
    if (it == end() || comp_(value, *it)) {
      return end();
    }
    return it;
  }

  const Other& at_other(const T& key) const {
    iterator it = find(key);
    if (it == end()) {
      throw std::out_of_range("key is not contained in the container");
    } else {
      return *it.flip();
    }
  }

  // returns true if successfully inserted bimap_node
  bool insert_node(node* ins, const T* value = nullptr) {
    if (value == nullptr) {
      value = &as_val(ins);
    }
    node* prev = sentinel_;
    node** nd = &(prev->p_);
    const Compare& comp = comp_;
    bool greatest = true;
    bool lowest = true;
    while (*nd != nullptr) {
      if (comp(*value, as_val(*nd))) {
        prev = *nd;
        nd = &(prev->l_);
        greatest = false;
      } else if (comp(as_val(*nd), *value)) {
        prev = *nd;
        nd = &(prev->r_);
        lowest = false;
      } else {
        return false;
      }
    }
    if (greatest) {
      sentinel_->l_ = ins;
    }
    if (lowest) {
      sentinel_->r_ = ins;
    }
    *nd = ins;
    ins->p_ = prev;
    return true;
  }

  void insert_before(node* ins, node* neighbour) {
    if (neighbour == sentinel_) {
      if (sentinel_->l_ == sentinel_) {
        sentinel_->l_ = ins;
        sentinel_->r_ = ins;
        sentinel_->p_ = ins;
        ins->p_ = sentinel_;
      } else {
        sentinel_->l_->r_ = ins;
        ins->p_ = sentinel_->l_;
        sentinel_->l_ = ins;
      }
    } else if (neighbour->l_ == nullptr) {
      neighbour->l_ = ins;
      ins->p_ = neighbour;
      if (sentinel_->r_ == neighbour) {
        sentinel_->r_ = ins;
      }
    } else {
      node* prev_node = node::get_max(neighbour->l_);
      prev_node->r_ = ins;
      ins->p_ = prev_node;
    }
  }

  static void swap_sentinel(node* sl, node* sr) noexcept {
    std::swap(sl->p_, sr->p_);
    std::swap(sl->l_, sr->l_);
    std::swap(sl->r_, sr->r_);
    if (sl->l_ == sr) {
      sl->l_ = sl;
      sl->r_ = sl;
    }
    if (sr->l_ == sl) {
      sr->l_ = sr;
      sr->r_ = sr;
    }
    if (sl->p_ != nullptr) {
      sl->p_->p_ = sl;
    }
    if (sr->p_ != nullptr) {
      sr->p_->p_ = sr;
    }
  }

public:
  base_node<Tag>* sentinel_;
  [[no_unique_address]] Compare comp_;
};

} // namespace bimap_components

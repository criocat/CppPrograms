#pragma once

#include "map.h"

template <
    typename Left,
    typename Right,
    typename CompareLeft = std::less<Left>,
    typename CompareRight = std::less<Right>>
class bimap
    : bimap_components::map<Left, Right, CompareLeft, bimap_components::tag_left>
    , bimap_components::map<Left, Right, CompareRight, bimap_components::tag_right> {
  using LeftMap = bimap_components::map<Left, Right, CompareLeft, bimap_components::tag_left>;
  using RightMap = bimap_components::map<Left, Right, CompareRight, bimap_components::tag_right>;
  using ValueNode = bimap_components::value_node<Left, Right>;

public:
  using left_iterator = LeftMap::iterator;

  using right_iterator = RightMap::iterator;

private:
  template <typename Tag>
  using tag_type_v = std::conditional_t<std::is_same_v<Tag, bimap_components::tag_left>, Left, Right>;

  template <typename Tag>
  using tag_other_v = std::conditional_t<
      std::is_same_v<Tag, bimap_components::tag_left>,
      bimap_components::tag_right,
      bimap_components::tag_left>;

  template <typename Tag>
  using base_iterator =
      std::conditional_t<std::is_same_v<Tag, bimap_components::tag_left>, left_iterator, right_iterator>;

  template <typename Tag>
  using base_map_v = std::conditional_t<std::is_same_v<Tag, bimap_components::tag_left>, LeftMap, RightMap>;

  template <typename Tag>
  bool equal_value(const tag_type_v<Tag>& l, const tag_type_v<Tag>& r) const {
    return !base_map_v<Tag>::comp_(l, r) && !base_map_v<Tag>::comp_(r, l);
  }

  template <typename Tag>
  base_iterator<Tag> erase_iterator(base_iterator<Tag> it) noexcept {
    sz_--;
    base_iterator<Tag> res = base_map_v<Tag>::erase(it.nd_);
    base_map_v<tag_other_v<Tag>>::erase(it.flip().nd_);
    delete static_cast<ValueNode*>(it.nd_);
    return res;
  }

  template <typename Tag>
  bool erase_value(const tag_type_v<Tag>& key) {
    base_iterator<Tag> it = base_map_v<Tag>::find(key);
    if (it == base_map_v<Tag>::end()) {
      return false;
    }
    erase_iterator<Tag>(it);
    return true;
  }

  template <typename Tag>
  const tag_type_v<tag_other_v<Tag>>& at_other_or_default(const tag_type_v<Tag>& key) {
    base_iterator<Tag> it = base_map_v<Tag>::lower_bound(key);
    if (it != base_map_v<Tag>::end() && equal_value<Tag>(*it, key)) {
      return *it.flip();
    }
    auto default_value = tag_type_v<tag_other_v<Tag>>();
    base_iterator<tag_other_v<Tag>> it2 = base_map_v<tag_other_v<Tag>>::find(default_value);
    if (it2 == base_map_v<tag_other_v<Tag>>::end()) {
      if constexpr (std::is_same_v<Tag, bimap_components::tag_left>) {
        return *insert(key, std::move(default_value)).flip();
      } else {
        return *insert(std::move(default_value), key);
      }
    } else {
      bimap_components::base_node<Tag>* cur_node = it2.flip().nd_;
      ValueNode* new_node;
      if constexpr (std::is_same_v<Tag, bimap_components::tag_left>) {
        new_node = new ValueNode(key, tag_type_v<tag_other_v<Tag>>());
      } else {
        new_node = new ValueNode(tag_type_v<tag_other_v<Tag>>(), key);
      }
      base_map_v<Tag>::insert_before(new_node, it.nd_);
      base_map_v<Tag>::erase(cur_node);
      base_map_v<tag_other_v<Tag>>::move_node(it2.nd_, new_node);
      delete static_cast<ValueNode*>(cur_node);
      return *base_iterator<tag_other_v<Tag>>(new_node);
    }
  }

  template <typename L, typename R>
  left_iterator base_insert(L&& left, R&& right) {
    left_iterator itl = lower_bound_left(left);
    right_iterator itr = lower_bound_right(right);

    if ((itl != end_left() && equal_value<bimap_components::tag_left>(*itl, left)) ||
        (itr != end_right() && equal_value<bimap_components::tag_right>(*itr, right))) {
      return end_left();
    }
    auto* nd = new ValueNode(std::forward<L>(left), std::forward<R>(right));
    LeftMap::insert_before(nd, itl.nd_);
    RightMap::insert_before(nd, itr.nd_);
    sz_++;
    return left_iterator(nd);
  }

public:
  bimap(CompareLeft compare_left = CompareLeft(), CompareRight compare_right = CompareRight())
      : LeftMap(&sentinel_, std::move(compare_left))
      , RightMap(&sentinel_, std::move(compare_right))
      , sentinel_(true)
      , sz_(0) {}

  bimap(const bimap& other)
      : bimap(static_cast<const LeftMap&>(other).comp_, static_cast<const RightMap&>(other).comp_) {
    for (left_iterator it = other.begin_left(); it != other.end_left(); it++) {
      insert(*it, *it.flip());
    }
  }

  bimap(bimap&& other) noexcept
      : bimap(std::move(static_cast<LeftMap&>(other).comp_), std::move(static_cast<RightMap&>(other).comp_)) {
    std::swap(sz_, other.sz_);
    LeftMap::swap_sentinel(&sentinel_, &other.sentinel_);
    RightMap::swap_sentinel(&sentinel_, &other.sentinel_);
  }

  bimap& operator=(const bimap& other) {
    if (this == &other) {
      return *this;
    }
    bimap temp = other;
    swap(*this, temp);
    return *this;
  }

  bimap& operator=(bimap&& other) noexcept {
    if (this == &other) {
      return *this;
    }
    bimap temp = std::move(other);
    swap(*this, temp);
    return *this;
  }

  ~bimap() {
    erase_left(begin_left(), end_left());
  }

  friend void swap(bimap& l, bimap& r) noexcept {
    using std::swap;
    swap(l.sz_, r.sz_);
    swap(static_cast<LeftMap&>(l).comp_, static_cast<LeftMap&>(r).comp_);
    swap(static_cast<RightMap&>(l).comp_, static_cast<RightMap&>(r).comp_);
    LeftMap::swap_sentinel(&l.sentinel_, &r.sentinel_);
    RightMap::swap_sentinel(&l.sentinel_, &r.sentinel_);
  }

  left_iterator insert(const Left& left, const Right& right) {
    return base_insert(left, right);
  }

  left_iterator insert(const Left& left, Right&& right) {
    return base_insert(left, std::move(right));
  }

  left_iterator insert(Left&& left, const Right& right) {
    return base_insert(std::move(left), right);
  }

  left_iterator insert(Left&& left, Right&& right) {
    return base_insert(std::move(left), std::move(right));
  }

  left_iterator erase_left(left_iterator it) noexcept {
    return erase_iterator<bimap_components::tag_left>(it);
  }

  right_iterator erase_right(right_iterator it) noexcept {
    return erase_iterator<bimap_components::tag_right>(it);
  }

  bool erase_left(const Left& left) {
    return erase_value<bimap_components::tag_left>(left);
  }

  bool erase_right(const Right& right) {
    return erase_value<bimap_components::tag_right>(right);
  }

  left_iterator erase_left(left_iterator first, left_iterator last) noexcept {
    while (first != last) {
      first = erase_left(first);
    }
    return last;
  }

  right_iterator erase_right(right_iterator first, right_iterator last) noexcept {
    while (first != last) {
      first = erase_right(first);
    }
    return last;
  }

  left_iterator find_left(const Left& left) const {
    return LeftMap::find(left);
  }

  right_iterator find_right(const Right& right) const {
    return RightMap::find(right);
  }

  const Right& at_left(const Left& key) const {
    return LeftMap::at_other(key);
  }

  const Left& at_right(const Right& key) const {
    return RightMap::at_other(key);
  }

  const Right& at_left_or_default(const Left& key)
    requires std::is_default_constructible_v<Right>
  {
    return at_other_or_default<bimap_components::tag_left>(key);
  }

  const Left& at_right_or_default(const Right& key)
    requires std::is_default_constructible_v<Left>
  {
    return at_other_or_default<bimap_components::tag_right>(key);
  }

  left_iterator lower_bound_left(const Left& left) const {
    return LeftMap::lower_bound(left);
  }

  left_iterator upper_bound_left(const Left& left) const {
    return LeftMap::upper_bound(left);
  }

  right_iterator lower_bound_right(const Right& right) const {
    return RightMap::lower_bound(right);
  }

  right_iterator upper_bound_right(const Right& right) const {
    return RightMap::upper_bound(right);
  }

  left_iterator begin_left() const noexcept {
    return LeftMap::begin();
  }

  left_iterator end_left() const noexcept {
    return LeftMap::end();
  }

  right_iterator begin_right() const noexcept {
    return RightMap::begin();
  }

  right_iterator end_right() const noexcept {
    return RightMap::end();
  }

  bool empty() const noexcept {
    return size() == 0;
  }

  std::size_t size() const noexcept {
    return sz_;
  }

  friend bool operator==(const bimap& l, const bimap& r) {
    if (l.size() != r.size()) {
      return false;
    }
    left_iterator itl = l.begin_left();
    left_iterator itr = r.begin_left();
    for (size_t i = 0; i < l.size(); ++i) {
      if (static_cast<const LeftMap&>(l).comp_(*itl, *itr) || static_cast<const LeftMap&>(l).comp_(*itr, *itl) ||
          static_cast<const RightMap&>(l).comp_(*itl.flip(), *itr.flip()) ||
          static_cast<const RightMap&>(l).comp_(*itr.flip(), *itl.flip())) {
        return false;
      }
      ++itl;
      ++itr;
    }
    return true;
  }

  friend bool operator!=(const bimap& l, const bimap& r) {
    return !(l == r);
  }

private:
  bimap_components::bimap_node sentinel_;
  size_t sz_;
};

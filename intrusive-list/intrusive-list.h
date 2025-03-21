#pragma once

#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

namespace intrusive {

template <typename T, typename Tag>
class list;

struct list_element_base {
private:
  list_element_base* prev;
  list_element_base* next;

public:
  template <typename T, typename Tag>
  friend class list;

  list_element_base()
      : prev(this)
      , next(this) {}

  list_element_base(list_element_base&& other)
      : list_element_base() {
    *this = std::move(other);
  }

  list_element_base(const list_element_base&)
      : list_element_base() {}

  ~list_element_base();

  bool is_linked() const noexcept;

  void unlink() noexcept;

  list_element_base& operator=(list_element_base&& other) noexcept;

  list_element_base& operator=(const list_element_base& other) noexcept;

  void link_before(list_element_base* node) noexcept;
};

class default_tag;

template <typename Tag = default_tag>
class list_element : private list_element_base {
  template <typename Y, typename Tg>
  friend class list;
};

template <typename T, typename Tag = default_tag>
class list {
  static_assert(std::is_base_of_v<list_element<Tag>, T>, "T must derive from list_element");

  using node = list_element_base;
  using tag_node = list_element<Tag>;

private:
  template <typename G>
  struct base_iterator {
  public:
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = G*;
    using reference = G&;
    using iterator_category = std::bidirectional_iterator_tag;

  private:
    friend list;

    base_iterator(node* cur_node) noexcept
        : nd_(cur_node) {}

    reference as_val(node* nd) const noexcept {
      return *static_cast<pointer>(static_cast<tag_node*>(nd));
    }

  public:
    base_iterator() noexcept
        : nd_(nullptr) {}

    reference operator*() const {
      return as_val(nd_);
    }

    operator base_iterator<const G>() const noexcept {
      return {nd_};
    }

    pointer operator->() const {
      return &as_val(nd_);
    }

    base_iterator& operator++() {
      nd_ = nd_->next;
      return *this;
    }

    base_iterator operator++(int) {
      base_iterator tmp = *this;
      ++*this;
      return tmp;
    }

    base_iterator& operator--() {
      nd_ = nd_->prev;
      return *this;
    }

    base_iterator operator--(int) {
      base_iterator tmp = *this;
      --*this;
      return tmp;
    }

    friend bool operator==(const base_iterator& lhs, const base_iterator& rhs) noexcept {
      return lhs.nd_ == rhs.nd_;
    }

    friend bool operator!=(const base_iterator& lhs, const base_iterator& rhs) noexcept {
      return !(lhs == rhs);
    }

  private:
    node* nd_;
  };

public:
  using iterator = base_iterator<T>;
  using const_iterator = base_iterator<const T>;

private:
  T& as_val(node* nd) const noexcept {
    return *static_cast<T*>(static_cast<tag_node*>(nd));
  }

  node* as_node(T& val) const noexcept {
    return static_cast<tag_node*>(&val);
  }

public:
  list() noexcept = default;

  ~list() = default;

  list(const list&) = delete;
  list& operator=(const list&) = delete;

  list(list&& other) noexcept = default;

  list& operator=(list&& other) noexcept = default;

  bool empty() const noexcept {
    return !sentinel_.is_linked();
  }

  size_t size() const noexcept {
    return std::distance(begin(), end());
  }

  T& front() noexcept {
    return as_val(sentinel_.next);
  }

  const T& front() const noexcept {
    return as_val(sentinel_.next);
  }

  T& back() noexcept {
    return as_val(sentinel_.prev);
  }

  const T& back() const noexcept {
    return as_val(sentinel_.prev);
  }

  void push_front(T& value) noexcept {
    sentinel_.next->link_before(as_node(value));
  }

  void push_back(T& value) noexcept {
    sentinel_.link_before(static_cast<tag_node*>(as_node(value)));
  }

  void pop_front() noexcept {
    sentinel_.next->unlink();
  }

  void pop_back() noexcept {
    sentinel_.prev->unlink();
  }

  void clear() noexcept {
    sentinel_.unlink();
  }

  iterator begin() noexcept {
    return iterator(sentinel_.next);
  }

  const_iterator begin() const noexcept {
    return const_iterator(sentinel_.next);
  }

  iterator end() noexcept {
    return iterator(const_cast<node*>(&sentinel_));
  }

  const_iterator end() const noexcept {
    return const_iterator(const_cast<node*>(&sentinel_));
  }

  iterator insert(const_iterator pos, T& value) noexcept {
    pos.nd_->link_before(as_node(value));
    return iterator(pos.nd_->prev);
  }

  iterator erase(const_iterator pos) noexcept {
    if (pos.nd_ == &sentinel_) {
      return end();
    }
    node* res = pos.nd_->next;
    pos.nd_->unlink();
    return iterator(res);
  }

  void splice(const_iterator pos, list&, const_iterator first, const_iterator last) noexcept {
    if (first == last) {
      return;
    }
    node* rnode = last.nd_->prev;
    link(first.nd_->prev, rnode->next);
    link(pos.nd_->prev, first.nd_);
    link(rnode, pos.nd_);
  }

private:
  void link(node* l, node* r) noexcept {
    l->next = r;
    r->prev = l;
  }

private:
  node sentinel_;
};

} // namespace intrusive

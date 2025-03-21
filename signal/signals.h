#pragma once

#include "intrusive-list.h"

#include <functional>

namespace signals {

template <typename T>
class signal;

struct signal_tag;

template <typename T>
class Connection;

template <typename T>
class emit_scope_guard;

class box : public intrusive::list_element<signal_tag> {
  template <typename T>
  friend class signal;

  template <typename T>
  friend class emit_scope_guard;

  template <typename T>
  friend class Connection;

private:
  box()
      : is_sentinel(false) {}

  box(bool)
      : is_sentinel(true) {}

  bool is_sentinel;
};

template <typename... Args>
class Connection<void(Args...)> : public box {
  using Func = void(Args...);
  using slot = std::function<Func>;

  template <typename T>
  friend class signal;

  template <typename T>
  friend class emit_scope_guard;

public:
  Connection() noexcept
      : signal_alive_(false) {}

  Connection(Connection&& other) noexcept
      : Connection() {
    *this = std::move(other);
  }

  Connection& operator=(Connection&& other) noexcept {
    if (this == &other) {
      return *this;
    }
    disconnect();
    other.change_emit_it(this);
    sl_ = std::move(other.sl_);
    static_cast<list_element&>(*this) = std::move(other);
    signal_alive_ = other.signal_alive_;
    other.signal_alive_ = false;
    return *this;
  }

  void disconnect() noexcept {
    using iterator = typename intrusive::list<Connection, signal_tag>::iterator;
    if (!intrusive::list_element<signal_tag>::is_linked() || !signal_alive_) {
      return;
    }
    change_emit_it((--iterator(this)).nd_);
    unlink();
    signal_alive_ = false;
  }

  ~Connection() {
    disconnect();
  }

private:
  Connection(signal<Func>& sig, slot&& sl)
      : signal_alive_(true)
      , sl_(std::move(sl)) {
    sig.link_before(this);
  }

  void change_emit_it(intrusive::list_element_base* new_node) noexcept {
    if (!intrusive::list_element<signal_tag>::is_linked() || !signal_alive_) {
      return;
    }
    using iterator = typename intrusive::list<Connection, signal_tag>::iterator;
    iterator it(this);
    while (!static_cast<box*>(it.nd_)->is_sentinel) {
      it++;
    }
    const signal<Func>* sig_ptr = static_cast<signal<Func>*>(it.nd_);
    emit_scope_guard<Func>* last = sig_ptr->last_quard_;
    while (last != nullptr) {
      if (iterator(this) == last->it) {
        last->it = iterator(new_node);
      }
      last = last->pref;
    }
  }

private:
  bool signal_alive_;
  slot sl_;
};

template <typename... Args>
class emit_scope_guard<void(Args...)> {
  using Func = void(Args...);
  using iterator = typename intrusive::list<Connection<Func>, signal_tag>::const_iterator;

public:
  emit_scope_guard(signal<Func>& sig) noexcept
      : signal_alive(true)
      , it(++iterator(&sig))
      , pref(sig.last_quard_)
      , sig(&sig) {
    sig.last_quard_ = this;
  }

  ~emit_scope_guard() {
    if (!signal_alive) {
      if (pref != nullptr) {
        pref->signal_alive = false;
      }
    } else {
      sig->last_quard_ = pref;
      if (pref != nullptr) {
        pref->sig = sig;
      }
    }
  }

public:
  bool signal_alive;
  iterator it;
  emit_scope_guard* pref;
  signal<Func>* sig;
};

template <typename... Args>
class signal<void(Args...)> : public box {
  using Func = void(Args...);
  using slot = std::function<Func>;
  using iterator = typename intrusive::list<Connection<Func>, signal_tag>::iterator;

  template <typename T>
  friend class Connection;

  template <typename T>
  friend class emit_scope_guard;

public:
  using connection = Connection<Func>;

  signal() noexcept
      : box(true)
      , last_quard_(nullptr) {}

  signal(const signal&) = delete;
  signal& operator=(const signal&) = delete;

  signal(signal&& other) noexcept
      : signal() {
    *this = std::move(other);
  }

  signal& operator=(signal&& other) noexcept {
    if (this == &other) {
      return *this;
    }
    clear_signal();
    if (other.last_quard_ != nullptr) {
      other.last_quard_->sig = this;
    }
    std::swap(last_quard_, other.last_quard_);
    static_cast<list_element_base&>(*this) = std::move(other);
    return *this;
  }

  ~signal() {
    clear_signal();
  }

  connection connect(slot slot) noexcept {
    return connection(*this, std::move(slot));
  }

  void operator()(Args... args) const {
    emit_scope_guard<Func> guard(const_cast<signal&>(*this));
    while (guard.it != iterator(guard.sig)) {
      guard.it->sl_(args...);
      if (!guard.signal_alive) {
        break;
      }
      guard.it++;
    }
  }

private:
  void clear_signal() {
    iterator it(this);
    it++;
    while (it != iterator(this)) {
      (*it).signal_alive_ = false;
      it++;
    }
    while (last_quard_ != nullptr) {
      last_quard_->signal_alive = false;
      last_quard_ = last_quard_->pref;
    }
    unlink();
  }

private:
  emit_scope_guard<Func>* last_quard_;
};

} // namespace signals

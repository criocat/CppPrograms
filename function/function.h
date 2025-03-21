#pragma once

#include <algorithm>
#include <cstddef>
#include <memory>

class bad_function_call : public std::runtime_error {
public:
  explicit bad_function_call(const char* message)
      : std::runtime_error(message) {}
};

template <typename F>
class function;

template <typename R, typename... Args>
class function<R(Args...)> {
private:
  static constexpr std::size_t SMALL_SIZE = std::max(sizeof(void*) * 4, alignof(std::max_align_t));

  template <typename F>
  static constexpr bool small_object =
      sizeof(F) <= SMALL_SIZE && std::is_nothrow_move_constructible_v<F> && alignof(F) <= alignof(std::max_align_t);

  struct storage {
    alignas(std::max_align_t) std::byte data[SMALL_SIZE];
  };

  template <typename F>
  static F* get(storage& data) noexcept {
    if constexpr (small_object<F>) {
      return std::launder(reinterpret_cast<F*>(&data.data));
    } else {
      return *std::launder(reinterpret_cast<F**>(&data.data));
    }
  }

  struct operations_base {
    virtual R call(storage& data, Args&&... args) const = 0;

    virtual void copy(storage& from, storage& to) const = 0;

    virtual void move(storage& from, storage& to) const noexcept = 0;

    virtual void destroy(storage& data) const noexcept = 0;
  };

  class operations_default : operations_base {
    friend function;

    R call(storage&, Args&&...) const override {
      throw bad_function_call("calling an empty function");
    }

    void destroy(storage&) const noexcept override {}

    void copy(storage&, storage&) const override {}

    void move(storage&, storage&) const noexcept override {}
  };

  template <typename F>
  class operations : operations_base {
    friend function;

  public:
    void construct(storage& data, F&& func) const {
      std::construct_at(reinterpret_cast<F**>(&data.data[0]), new F(std::move(func)));
    }

    R call(storage& data, Args&&... args) const override {
      return (*get<F>(data))(std::forward<Args>(args)...);
    }

    void destroy(storage& data) const noexcept override {
      delete get<F>(data);
    }

    void copy(storage& from, storage& to) const override {
      F* ptr = new F(*get<F>(from));
      std::construct_at(reinterpret_cast<F**>(&to.data[0]), ptr);
    }

    void move(storage& from, storage& to) const noexcept override {
      std::construct_at(reinterpret_cast<F**>(&to.data[0]), get<F>(from));
    }
  };

  template <typename F>
    requires (small_object<F>)
  class operations<F> : operations_base {
  private:
    friend function;

  public:
    void construct(storage& data, F&& func) const {
      std::construct_at(reinterpret_cast<F*>(&data.data[0]), std::move(func));
    }

    R call(storage& data, Args&&... args) const override {
      return (*get<F>(data))(std::forward<Args>(args)...);
    }

    void destroy(storage& data) const noexcept override {
      std::destroy_at(get<F>(data));
    }

    void copy(storage& from, storage& to) const override {
      std::construct_at(reinterpret_cast<F*>(&to.data[0]), *get<F>(from));
    }

    void move(storage& from, storage& to) const noexcept override {
      construct(to, std::move(*get<F>(from)));
      get<F>(from)->~F();
    }
  };

public:
  function() noexcept
      : ops_(&DEFAULT_OPS) {}

  template <typename F>
  function(F func)
      : ops_(&OPS<F>) {
    OPS<F>.construct(storage_, std::move(func));
  }

  function(const function& other)
      : ops_(other.ops_) {
    other.ops_->copy(other.storage_, storage_);
  }

  function(function&& other) noexcept
      : ops_(other.ops_) {
    other.ops_->move(other.storage_, storage_);
    other.ops_ = &DEFAULT_OPS;
  }

  function& operator=(const function& other) {
    function temp(other);
    return (*this = std::move(temp));
  }

  function& operator=(function&& other) noexcept {
    if (this == &other) {
      return *this;
    }
    ops_->destroy(storage_);
    other.ops_->move(other.storage_, storage_);
    ops_ = other.ops_;
    other.ops_ = &DEFAULT_OPS;
    return *this;
  }

  ~function() {
    ops_->destroy(storage_);
  }

  explicit operator bool() const noexcept {
    return ops_ != &DEFAULT_OPS;
  }

  R operator()(Args... args) const {
    return ops_->call(storage_, std::forward<Args>(args)...);
  }

  template <typename T>
  T* target() noexcept {
    return const_cast<T*>(static_cast<const function<R(Args...)>*>(this)->template target<T>());
  }

  template <typename T>
  const T* target() const noexcept {
    return (ops_ == &OPS<T> ? get<T>(storage_) : nullptr);
  }

private:
  mutable storage storage_;
  operations_base* ops_;

  template <typename F>
  inline static operations<F> OPS;

  inline static operations_default DEFAULT_OPS;
};

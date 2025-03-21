#include "enable_classes.h"

#include <compare>
#include <memory>
#include <stdexcept>
#include <utility>

struct nullopt_t {
  constexpr explicit nullopt_t(int) noexcept {}
};

inline constexpr nullopt_t nullopt(228);

struct in_place_t {
  constexpr explicit in_place_t() noexcept = default;
};

inline constexpr in_place_t in_place{};

template <typename T, bool trivial = std::is_trivially_destructible_v<T>>
struct destruct_base {
  union {
    T obj;
  };

  bool is_active;

  constexpr destruct_base() noexcept
      : is_active(false) {}

  constexpr destruct_base(destruct_base&&) noexcept = default;

  constexpr destruct_base(const destruct_base&) noexcept = default;

  constexpr destruct_base& operator=(const destruct_base&) noexcept = default;

  constexpr destruct_base& operator=(destruct_base&&) noexcept = default;

  constexpr void reset() noexcept {
    if (is_active) {
      std::destroy_at(std::addressof(obj));
      is_active = false;
    }
  }

  constexpr ~destruct_base() {
    reset();
  }
};

template <typename T>
struct destruct_base<T, true> {
  union {
    T obj;
  };

  bool is_active;

  constexpr destruct_base() noexcept
      : is_active(false) {}

  constexpr destruct_base(destruct_base&&) noexcept = default;

  constexpr destruct_base(const destruct_base&) noexcept = default;

  constexpr destruct_base& operator=(const destruct_base&) noexcept = default;

  constexpr destruct_base& operator=(destruct_base&&) noexcept = default;

  constexpr void reset() noexcept {
    is_active = false;
  }

  constexpr ~destruct_base() = default;
};

template <typename T>
struct optional_base : destruct_base<T> {
  using destruct_base<T>::destruct_base;

  template <typename U, std::enable_if<std::is_same_v<std::remove_cvref_t<U>, T>, bool> = true>
  constexpr optional_base(U&& x) noexcept(std::is_nothrow_move_constructible_v<T>) {
    this->construct_value(std::forward<U>(x));
  }

  constexpr T& operator*() & noexcept {
    return this->obj;
  }

  constexpr const T& operator*() const& noexcept {
    return this->obj;
  }

  constexpr T&& operator*() && noexcept {
    return std::move(this->obj);
  }

  constexpr const T&& operator*() const&& noexcept {
    return std::move(this->obj);
  }

  constexpr T* operator->() noexcept {
    return std::addressof(this->obj);
  }

  constexpr const T* operator->() const noexcept {
    return std::addressof(this->obj);
  }

  constexpr T& value() {
    if (!has_value()) {
      throw std::runtime_error("Empty");
    }
    return this->obj;
  }

  constexpr bool has_value() const noexcept {
    return this->is_active;
  }

protected:
  template <typename... Args>
  constexpr void construct_value(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) {
    std::construct_at(std::addressof(this->obj), std::forward<Args>(args)...);
    this->is_active = true;
  }
};

template <typename T, bool trivial = std::is_trivially_copy_constructible_v<T>>
struct copy_constructible_base : optional_base<T> {
  using optional_base<T>::optional_base;

  constexpr copy_constructible_base() = default;

  constexpr copy_constructible_base(copy_constructible_base&&) = default;

  constexpr copy_constructible_base& operator=(const copy_constructible_base&) = default;

  constexpr copy_constructible_base& operator=(copy_constructible_base&&) = default;

  constexpr copy_constructible_base(const copy_constructible_base& other
  ) noexcept(std::is_nothrow_copy_constructible_v<T>)
      : optional_base<T>() {
    if (other.has_value()) {
      this->construct_value(*other);
    }
  }
};

template <typename T>
struct copy_constructible_base<T, true> : optional_base<T> {
  using optional_base<T>::optional_base;
};

template <typename T, bool trivial = std::is_trivially_move_constructible_v<T>>
struct move_constructible_base : copy_constructible_base<T> {
  using copy_constructible_base<T>::copy_constructible_base;

  constexpr move_constructible_base() = default;

  constexpr move_constructible_base(const move_constructible_base&) = default;

  constexpr move_constructible_base& operator=(const move_constructible_base&) = default;

  constexpr move_constructible_base& operator=(move_constructible_base&&) = default;

  constexpr move_constructible_base(move_constructible_base&& other) noexcept(std::is_nothrow_move_constructible_v<T>)
      : copy_constructible_base<T>() {
    if (other.has_value()) {
      this->construct_value(std::move(*other));
    }
  }
};

template <typename T>
struct move_constructible_base<T, true> : copy_constructible_base<T> {
  using copy_constructible_base<T>::copy_constructible_base;
};

template <
    typename T,
    bool trivial = std::is_trivially_copy_assignable_v<T> && std::is_trivially_copy_constructible_v<T>>
struct copy_assignable_base : move_constructible_base<T> {
  using move_constructible_base<T>::move_constructible_base;

  constexpr copy_assignable_base() = default;

  constexpr copy_assignable_base(const copy_assignable_base& other) = default;

  constexpr copy_assignable_base(copy_assignable_base&&) = default;

  constexpr copy_assignable_base& operator=(copy_assignable_base&&) = default;

  constexpr copy_assignable_base& operator=(const copy_assignable_base& other
  ) noexcept(std::is_nothrow_copy_constructible_v<T> && std::is_nothrow_copy_assignable_v<T>) {
    if (!other.has_value()) {
      this->reset();
      return *this;
    }
    if (this->has_value()) {
      **this = *other;
    } else {
      this->construct_value(*other);
    }
    return *this;
  }
};

template <typename T>
struct copy_assignable_base<T, true> : move_constructible_base<T> {
  using move_constructible_base<T>::move_constructible_base;
};

template <
    typename T,
    bool trivial = std::is_trivially_move_assignable_v<T> && std::is_trivially_move_constructible_v<T>>
struct move_assignable_base : copy_assignable_base<T> {
  using copy_assignable_base<T>::copy_assignable_base;

  constexpr move_assignable_base() = default;

  constexpr move_assignable_base(const move_assignable_base& other) = default;

  constexpr move_assignable_base(move_assignable_base&&) = default;

  constexpr move_assignable_base& operator=(const move_assignable_base&) = default;

  constexpr move_assignable_base& operator=(move_assignable_base&& other
  ) noexcept(std::is_nothrow_move_constructible_v<T> && std::is_nothrow_move_assignable_v<T>) {
    if (!other.has_value()) {
      this->reset();
      return *this;
    }
    if (this->has_value()) {
      **this = std::move(*other);
    } else {
      this->construct_value(std::move(*other));
    }
    return *this;
  }
};

template <typename T>
struct move_assignable_base<T, true> : copy_assignable_base<T> {
  using copy_assignable_base<T>::copy_assignable_base;
};

template <typename T>
struct optional
    : move_assignable_base<T>
    , enable_copy_constructible<T>
    , enable_move_constructible<T>
    , enable_copy_assignable<T>
    , enable_move_assignable<T> {
  using move_assignable_base<T>::move_assignable_base;
  using value_type = T;

  constexpr void swap(optional& other
  ) noexcept(std::is_nothrow_swappable_v<T> && std::is_nothrow_move_constructible_v<T>) {
    if (other.has_value()) {
      if (this->has_value()) {
        using std::swap;
        swap(**this, *other);
      } else {
        using std::swap;
        this->construct_value(std::move(*other));
        other.reset();
      }
    } else {
      if (this->has_value()) {
        other.swap(*this);
      }
    }
  }

  constexpr optional(nullopt_t) noexcept
      : optional() {}

  template <typename... Args>
  constexpr T& emplace(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args&&...>) {
    this->reset();
    this->construct_value(std::forward<Args>(args)...);
    return **this;
  }

  template <typename... Args>
  explicit constexpr optional(in_place_t, Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args&&...>) {
    emplace(std::forward<Args>(args)...);
  }

  constexpr optional& operator=(nullopt_t) noexcept {
    this->reset();
    return *this;
  }

  template <
      typename U = T,
      std::enable_if_t<
          std::is_constructible_v<T, U&&> && !std::is_same_v<std::remove_cvref_t<U>, in_place_t> &&
              !std::is_same_v<std::remove_cvref_t<U>, optional<T>> &&
              (!std::is_same_v<bool, std::remove_cvref<T>> || !std::is_base_of_v<optional<T>, std::remove_cvref<U>>),
          bool> = true>
  constexpr explicit(!std::is_convertible_v<U&&, T>)
      optional(U&& value) noexcept(std::is_nothrow_constructible_v<T, U&&>) {
    this->construct_value(std::forward<U>(value));
  }

  template <typename U = T>
  constexpr std::enable_if_t<
      !std::is_same_v<std::remove_cvref_t<U>, optional<T>> && std::is_constructible_v<T, U> &&
          std::is_assignable_v<T&, U> && (!std::is_scalar_v<T> || !std::is_same_v<std::decay_t<U>, T>),
      optional&>
  operator=(U&& value) noexcept(std::is_nothrow_assignable_v<T&, U&&> && std::is_nothrow_constructible_v<T, U&&>) {
    if (this->has_value()) {
      this->obj = std::forward<U>(value);
    } else {
      this->construct_value(std::forward<U>(value));
    }
    return *this;
  }

  constexpr explicit operator bool() const noexcept {
    return this->has_value();
  }
};

template <typename T, std::enable_if_t<std::is_swappable_v<T> && std::is_move_constructible_v<T>, bool> = true>
constexpr void swap(
    optional<T>& lhs,
    optional<T>& rhs
) noexcept(std::is_nothrow_swappable_v<T> && std::is_nothrow_move_constructible_v<T>) {
  lhs.swap(rhs);
}

template <typename T, std::enable_if_t<!(std::is_swappable_v<T> && std::is_move_constructible_v<T>), bool> = true>
constexpr void swap(
    optional<T>& lhs,
    optional<T>& rhs
) noexcept(std::is_nothrow_swappable_v<T> && std::is_nothrow_move_constructible_v<T>) = delete;

template <typename T>
constexpr bool operator==(const optional<T>& lhs, const optional<T>& rhs) {
  if (bool(lhs) != bool(rhs)) {
    return false;
  }
  if (!bool(lhs)) {
    return true;
  }
  return *lhs == *rhs;
}

template <typename T>
constexpr bool operator!=(const optional<T>& lhs, const optional<T>& rhs) {
  if (bool(lhs) != bool(rhs)) {
    return true;
  }
  if (!bool(lhs)) {
    return false;
  }
  return *lhs != *rhs;
}

template <typename T>
constexpr bool operator<(const optional<T>& lhs, const optional<T>& rhs) {
  if (!bool(rhs)) {
    return false;
  }
  if (!bool(lhs)) {
    return true;
  }
  return *lhs < *rhs;
}

template <typename T>
constexpr bool operator<=(const optional<T>& lhs, const optional<T>& rhs) {
  if (!bool(lhs)) {
    return true;
  }
  if (!bool(rhs)) {
    return false;
  }
  return *lhs <= *rhs;
}

template <typename T>
constexpr bool operator>(const optional<T>& lhs, const optional<T>& rhs) {
  if (!bool(lhs)) {
    return false;
  }
  if (!bool(rhs)) {
    return true;
  }
  return *lhs > *rhs;
}

template <typename T>
constexpr bool operator>=(const optional<T>& lhs, const optional<T>& rhs) {
  if (!bool(rhs)) {
    return true;
  }
  if (!bool(lhs)) {
    return false;
  }
  return *lhs >= *rhs;
}

template <class T>
constexpr std::compare_three_way_result_t<T> operator<=>(const optional<T>& lhs, const optional<T>& rhs) {
  if (bool(lhs) && bool(rhs)) {
    return *lhs <=> *rhs;
  }
  return bool(lhs) <=> bool(rhs);
}

template <typename T>
optional(T) -> optional<T>;

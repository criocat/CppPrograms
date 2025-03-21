#pragma once

#include <type_traits>

template <typename T, bool trivial = std::is_copy_constructible_v<T>>
struct enable_copy_constructible {
  constexpr enable_copy_constructible(const enable_copy_constructible&) noexcept = delete;

  constexpr enable_copy_constructible() noexcept = default;

  constexpr enable_copy_constructible(enable_copy_constructible&&) noexcept = default;

  constexpr enable_copy_constructible& operator=(const enable_copy_constructible&) noexcept = default;

  constexpr enable_copy_constructible& operator=(enable_copy_constructible&&) noexcept = default;

  constexpr ~enable_copy_constructible() = default;
};

template <typename T>
struct enable_copy_constructible<T, true> {};

template <typename T, bool trivial = std::is_move_constructible_v<T>>
struct enable_move_constructible {
  constexpr enable_move_constructible() noexcept = default;

  constexpr enable_move_constructible(const enable_move_constructible&) noexcept = default;

  constexpr enable_move_constructible(enable_move_constructible&&) noexcept = delete;

  constexpr enable_move_constructible& operator=(const enable_move_constructible&) noexcept = default;

  constexpr enable_move_constructible& operator=(enable_move_constructible&&) noexcept = default;

  constexpr ~enable_move_constructible() = default;
};

template <typename T>
struct enable_move_constructible<T, true> {};

template <typename T, bool trivial = std::is_copy_assignable_v<T> && std::is_copy_constructible_v<T>>
struct enable_copy_assignable {
  constexpr enable_copy_assignable() noexcept = default;

  constexpr enable_copy_assignable(const enable_copy_assignable&) noexcept = default;

  constexpr enable_copy_assignable(enable_copy_assignable&&) noexcept = default;

  constexpr enable_copy_assignable& operator=(const enable_copy_assignable&) noexcept = delete;

  constexpr enable_copy_assignable& operator=(enable_copy_assignable&&) noexcept = default;

  constexpr ~enable_copy_assignable() = default;
};

template <typename T>
struct enable_copy_assignable<T, true> {};

template <typename T, bool trivial = std::is_move_assignable_v<T> && std::is_move_constructible_v<T>>
struct enable_move_assignable {
  constexpr enable_move_assignable() noexcept = default;

  constexpr enable_move_assignable(const enable_move_assignable&) noexcept = default;

  constexpr enable_move_assignable(enable_move_assignable&&) noexcept = default;

  constexpr enable_move_assignable& operator=(const enable_move_assignable&) noexcept = default;

  constexpr enable_move_assignable& operator=(enable_move_assignable&&) noexcept = delete;

  constexpr ~enable_move_assignable() = default;
};

template <typename T>
struct enable_move_assignable<T, true> {};

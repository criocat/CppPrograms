#pragma once

#include "variant_traits.h"

namespace detail {

template <typename... Args>
union UnionStorage {
  constexpr void clear(size_t) const {}

  constexpr void swap(size_t, UnionStorage&) const {}
};

template <typename T, typename... Tail>
union UnionStorage<T, Tail...> {
  T node;
  UnionStorage<Tail...> tail;

  template <template <typename> typename Pred>
  static constexpr bool is_all_v = is_all<Pred, T, Tail...>::value;

  constexpr UnionStorage()
      : tail() {}

  constexpr ~UnionStorage()
    requires (is_all_v<std::is_trivially_destructible>)
  = default;

  constexpr ~UnionStorage() {}

  constexpr UnionStorage(const UnionStorage&)
    requires (is_all_v<std::is_trivially_copy_constructible>)
  = default;

  constexpr UnionStorage(const UnionStorage&) = delete;

  constexpr UnionStorage(UnionStorage&&)
    requires (is_all_v<std::is_trivially_move_constructible>)
  = default;

  constexpr UnionStorage(UnionStorage&&) = delete;

  constexpr UnionStorage& operator=(const UnionStorage&)
    requires (is_all_v<std::is_trivially_copy_assignable>)
  = default;

  constexpr UnionStorage& operator=(const UnionStorage&) = delete;

  constexpr UnionStorage& operator=(UnionStorage&&)
    requires (is_all_v<std::is_trivially_move_assignable>)
  = default;

  constexpr UnionStorage& operator=(UnionStorage&&) = delete;

  template <size_t Index>
  constexpr auto& get() {
    if constexpr (Index != 0) {
      return tail.template get<Index - 1>();
    } else {
      return node;
    }
  }

  template <size_t Index>
  constexpr const auto& get() const {
    if constexpr (Index != 0) {
      return tail.template get<Index - 1>();
    } else {
      return node;
    }
  }

  constexpr void swap(size_t index, UnionStorage& storage) {
    if (index == 0) {
      using std::swap;
      swap(node, storage.node);
    } else {
      tail.swap(index - 1, storage.tail);
    }
  }

  constexpr void clear(size_t index) {
    if (index == 0) {
      std::destroy_at(std::addressof(node));
      std::construct_at(std::addressof(tail));
    } else {
      tail.clear(index - 1);
    }
  }
};

} // namespace detail

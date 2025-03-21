#pragma once

#include "union_storage.h"

template <typename Res, typename Visitor, typename... Types>
constexpr Res visit(Visitor&& vis, Types&&... vars);

namespace detail {

template <typename Res, typename Func>
class apply_lambda {
public:
  template <typename T1, typename T2>
  constexpr Res operator()(T1&& v1, T2&& v2) {
    if constexpr (!std::is_same_v<std::remove_cvref_t<T1>, std::remove_cvref_t<T2>>) {
      throw bad_variant_access("unrichible");
    } else {
      return Func{}(std::forward<T1>(v1), std::forward<T2>(v2));
    }
  }
};

template <typename Res, size_t... nums, typename Visitor, typename... Variants>
constexpr Res visit_cell(Visitor&&, Variants&&...);

} // namespace detail

template <typename... Types>
class variant {
  template <typename Res, size_t... nums, typename Visitor, typename... Variants>
  friend constexpr Res detail::visit_cell(Visitor&&, Variants&&...);

private:
  template <size_t Index>
  using nth_in_pack_t = detail::nth_in_pack<Index, Types...>::type;

  template <typename T>
  static constexpr size_t type_to_index_v = detail::type_to_index<T, Types...>::value;

  template <typename Ec>
  static constexpr size_t get_count_v = detail::get_count<Ec, Types...>::value;

  template <typename T>
  static constexpr bool exectly_one = get_count_v<T> == 1;

  template <typename T>
  static constexpr size_t func_type_index =
      decltype(detail::func_check<T, std::make_index_sequence<sizeof...(Types)>, Types...>::func(std::declval<T>())
      )::value;

  template <template <typename> typename Pred>
  static constexpr bool is_all_v = detail::is_all<Pred, Types...>::value;

  constexpr void copy_construct(const variant& other) {
    alternative = other.index();
    try {
      visit<void>(
          detail::apply_lambda<void, decltype([](const auto& a, const auto& b) {
                                 std::construct_at(std::addressof(detail::remove_ref_const(a)), b);
                               })>(),
          *this,
          other
      );
    } catch (...) {
      alternative = variant_npos;
      throw;
    }
  }

  constexpr void move_construct(variant& other) {
    alternative = other.index();
    try {
      visit<void>(
          detail::apply_lambda<void, decltype([](auto& a, auto& b) {
                                 std::construct_at(
                                     std::addressof(detail::remove_ref_const(a)),
                                     std::move(detail::remove_ref_const(b))
                                 );
                               })>(),
          *this,
          other
      );
    } catch (...) {
      alternative = variant_npos;
      throw;
    }
  }

  constexpr void copy_assign(const variant& other) {
    visit<void>(detail::apply_lambda<void, decltype([](auto& a, auto& b) { a = b; })>(), *this, other);
  }

  constexpr void move_assign(variant& other) {
    visit<void>(detail::apply_lambda<void, decltype([](auto& a, auto& b) { a = std::move(b); })>(), *this, other);
  }

  constexpr bool check_copy_assign() const {
    return visit<bool>(
        []<typename T>(T&&) {
          return std::is_nothrow_copy_constructible_v<std::remove_cvref_t<T>> ||
                 !std::is_nothrow_move_constructible_v<std::remove_cvref_t<T>>;
        },
        *this
    );
  }

public:
  template <size_t I, typename... Args>
    requires (I < sizeof...(Types) && std::is_constructible_v<nth_in_pack_t<I>, Args...>)
  constexpr explicit variant(
      in_place_index_t<I>,
      Args&&... args
  ) noexcept(std::is_nothrow_constructible_v<nth_in_pack_t<I>, Args...>)
      : alternative(variant_npos) {
    std::construct_at(
        std::addressof(detail::remove_ref_const(storage_.template get<I>())),
        std::forward<Args>(args)...
    );
    alternative = I;
  }

  template <size_t I, typename U, typename... Args>
    requires (I < sizeof...(Types) && std::is_constructible_v<nth_in_pack_t<I>, std::initializer_list<U>&, Args...>)
  constexpr explicit variant(
      in_place_index_t<I>,
      std::initializer_list<U> il,
      Args&&... args
  ) noexcept(std::is_nothrow_constructible_v<nth_in_pack_t<I>, std::initializer_list<U>&, Args...>)
      : alternative(variant_npos) {
    std::construct_at(
        std::addressof(detail::remove_ref_const(storage_.template get<I>())),
        il,
        std::forward<Args>(args)...
    );
    alternative = I;
  }

  template <typename T, typename... Args>
    requires (get_count_v<T> == 1 && std::is_constructible_v<T, Args...>)
  constexpr explicit variant(in_place_type_t<T>, Args&&... args) noexcept(
      noexcept(variant(in_place_index_t<type_to_index_v<T>>(), std::forward<Args>(args)...))
  )
      : variant(in_place_index_t<type_to_index_v<T>>(), std::forward<Args>(args)...) {}

  template <typename T, typename U, typename... Args>
    requires (get_count_v<T> == 1 && std::is_constructible_v<T, std::initializer_list<U>&, Args...>)
  constexpr explicit variant(in_place_type_t<T>, std::initializer_list<U> il, Args&&... args) noexcept(
      noexcept(variant(in_place_index_t<type_to_index_v<T>>(), il, std::forward<Args>(args)...))
  )
      : variant(in_place_index_t<type_to_index_v<T>>(), il, std::forward<Args>(args)...) {}

  template <typename T>
    requires (sizeof...(Types) > 0 && !std::is_same_v<std::remove_cvref_t<T>, variant> && detail::not_in_place<T> && detail::check_conversion<T, Types...> && exectly_one<nth_in_pack_t<func_type_index<T>>> && std::is_constructible_v<nth_in_pack_t<func_type_index<T>>, T>)
  constexpr variant(T&& val) noexcept(noexcept(variant(in_place_index_t<func_type_index<T>>(), std::forward<T>(val))))
      : variant(in_place_index_t<func_type_index<T>>(), std::forward<T>(val)) {}

  constexpr variant() noexcept(std::is_nothrow_default_constructible_v<nth_in_pack_t<0>>)
    requires (std::is_default_constructible_v<nth_in_pack_t<0>>)
      : variant(in_place_index_t<0>()) {}

  constexpr variant(const variant&) = delete;

  constexpr variant(const variant&)
    requires (is_all_v<std::is_trivially_copy_constructible>)
  = default;

  constexpr variant(const variant& other) noexcept(is_all_v<std::is_nothrow_copy_constructible>)
    requires (is_all_v<std::is_copy_constructible> && !is_all_v<std::is_trivially_copy_constructible>)
      : alternative(variant_npos) {
    if (!other.valueless_by_exception()) {
      alternative = other.index();
      copy_construct(other);
    }
  }

  constexpr variant(variant&&)
    requires (is_all_v<std::is_trivially_move_constructible>)
  = default;

  constexpr variant(variant&& other) noexcept(is_all_v<std::is_nothrow_move_constructible>)
    requires (is_all_v<std::is_move_constructible> && !is_all_v<std::is_trivially_move_constructible>)
      : alternative(variant_npos) {
    if (!other.valueless_by_exception()) {
      move_construct(other);
      alternative = other.index();
    }
  }

  constexpr variant& operator=(const variant&)
    requires (!(is_all_v<std::is_copy_assignable> && is_all_v<std::is_copy_constructible>) )
  = delete;

  constexpr variant& operator=(const variant&)
    requires (is_all_v<std::is_trivially_copy_assignable> && is_all_v<std::is_trivially_copy_constructible> &&
              is_all_v<std::is_trivially_destructible>)
  = default;

  constexpr variant& operator=(const variant& other
  ) noexcept(is_all_v<std::is_nothrow_copy_assignable> && is_all_v<std::is_nothrow_copy_constructible>)
    requires (!(is_all_v<std::is_trivially_copy_assignable> && is_all_v<std::is_trivially_copy_constructible> && is_all_v<std::is_trivially_destructible>) && is_all_v<std::is_copy_assignable> && is_all_v<std::is_copy_constructible>)
  {
    if (this == &other) {
      return *this;
    }
    if (other.valueless_by_exception()) {
      clear();
    } else if (other.index() == index()) {
      copy_assign(other);
    } else if (other.check_copy_assign()) {
      clear();
      alternative = other.index();
      copy_construct(other);
    } else {
      this->operator=(variant(other));
    }
    return *this;
  }

  constexpr variant& operator=(variant&&)
    requires (is_all_v<std::is_trivially_move_assignable> && is_all_v<std::is_trivially_move_constructible> &&
              is_all_v<std::is_trivially_destructible>)
  = default;

  constexpr variant& operator=(variant&& other
  ) noexcept(is_all_v<std::is_nothrow_move_assignable> && is_all_v<std::is_nothrow_move_constructible>)
    requires (!(is_all_v<std::is_trivially_move_assignable> && is_all_v<std::is_trivially_move_constructible> && is_all_v<std::is_trivially_destructible>) && is_all_v<std::is_move_assignable> && is_all_v<std::is_move_constructible>)
  {
    if (this == &other) {
      return *this;
    }
    if (other.valueless_by_exception()) {
      clear();
    } else if (other.index() == index()) {
      move_assign(other);
    } else {
      clear();
      move_construct(other);
      alternative = other.index();
    }
    return *this;
  }

  template <class T>
    requires (sizeof...(Types) > 0 && !std::is_same_v<std::remove_cvref_t<T>, variant> && detail::check_conversion<T, Types...> && exectly_one<nth_in_pack_t<func_type_index<T>>> && std::is_assignable_v<nth_in_pack_t<func_type_index<T>>&, T> && std::is_constructible_v<nth_in_pack_t<func_type_index<T>>, T>)
  constexpr variant& operator=(T&& value
  ) noexcept(std::is_nothrow_constructible_v<nth_in_pack_t<func_type_index<T>>, T> && std::is_nothrow_assignable_v<nth_in_pack_t<func_type_index<T>>&, T>) {
    if (alternative == func_type_index<T>) {
      storage_.template get<func_type_index<T>>() = std::forward<T>(value);
    } else {
      if constexpr (std::is_nothrow_constructible_v<nth_in_pack_t<func_type_index<T>>, T> ||
                    !std::is_nothrow_move_constructible_v<nth_in_pack_t<func_type_index<T>>>) {
        clear();
        std::construct_at(std::addressof(storage_.template get<func_type_index<T>>()), std::forward<T>(value));
      } else {
        nth_in_pack_t<func_type_index<T>> temp_value(std::forward<T>(value));
        clear();
        std::construct_at(std::addressof(storage_.template get<func_type_index<T>>()), std::move(temp_value));
      }
      alternative = func_type_index<T>;
    }
    return *this;
  }

  constexpr ~variant()
    requires (is_all_v<std::is_trivially_destructible>)
  = default;

  constexpr ~variant() {
    clear();
  }

  constexpr std::size_t index() const noexcept {
    return alternative;
  }

  constexpr bool valueless_by_exception() const noexcept {
    return index() == variant_npos;
  }

  template <size_t I, typename... Args>
    requires (I < sizeof...(Types) && std::is_constructible_v<nth_in_pack_t<I>, Args...>)
  constexpr variant_alternative_t<I, variant>& emplace(Args&&... args
  ) noexcept(std::is_nothrow_constructible_v<nth_in_pack_t<I>, Args...>) {
    clear();
    std::construct_at(
        std::addressof(detail::remove_ref_const(storage_.template get<I>())),
        std::forward<Args>(args)...
    );
    alternative = I;
    return storage_.template get<I>();
  }

  template <size_t I, typename U, typename... Args>
    requires (I < sizeof...(Types) && std::is_constructible_v<nth_in_pack_t<I>, std::initializer_list<U>&, Args...>)
  constexpr variant_alternative_t<I, variant>& emplace(
      std::initializer_list<U> il,
      Args&&... args
  ) noexcept(std::is_nothrow_constructible_v<nth_in_pack_t<I>, std::initializer_list<U>&, Args...>) {
    clear();
    std::construct_at(
        std::addressof(detail::remove_ref_const(storage_.template get<I>())),
        il,
        std::forward<Args>(args)...
    );
    alternative = I;
    return storage_.template get<I>();
  }

  template <typename T, typename... Args>
    requires (std::is_constructible_v<T, Args...> && exectly_one<T>)
  constexpr T& emplace(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) {
    emplace<type_to_index_v<T>>(std::forward<Args>(args)...);
    return storage_.template get<type_to_index_v<T>>();
  }

  template <typename T, typename U, typename... Args>
    requires (std::is_constructible_v<T, std::initializer_list<U>&, Args...> && exectly_one<T>)
  constexpr T& emplace(
      std::initializer_list<U> il,
      Args&&... args
  ) noexcept(std::is_nothrow_constructible_v<T, std::initializer_list<U>&, Args...>) {
    emplace<type_to_index_v<T>>(il, std::forward<Args>(args)...);
    return storage_.template get<type_to_index_v<T>>();
  }

public:
  constexpr void clear() {
    if (alternative != variant_npos) {
      storage_.clear(alternative);
      alternative = variant_npos;
    }
  }

  constexpr void swap(variant& other
  ) noexcept(is_all_v<std::is_nothrow_move_constructible> && is_all_v<std::is_nothrow_swappable>)
    requires (is_all_v<std::is_move_constructible> && is_all_v<std::is_swappable>)
  {
    if (valueless_by_exception() && other.valueless_by_exception()) {
    } else if (index() == other.index()) {
      storage_.swap(index(), other.storage_);
    } else if (valueless_by_exception()) {
      move_construct(other);
      alternative = other.index();
      other.clear();
    } else if (other.valueless_by_exception()) {
      other.swap(*this);
    } else {
      variant temp(std::move(other));
      other.clear();
      other.move_construct(*this);
      other.alternative = index();
      clear();
      move_construct(temp);
      alternative = temp.index();
    }
  }

  template <std::size_t Index>
    requires (Index < sizeof...(Types))
  friend constexpr variant_alternative_t<Index, variant>& get(variant& v) {
    if (v.index() != Index) {
      throw bad_variant_access("invalid index");
    }
    return v.storage_.template get<Index>();
  }

  template <std::size_t Index>
    requires (Index < sizeof...(Types))
  friend constexpr variant_alternative_t<Index, variant>&& get(variant&& v) {
    if (v.index() != Index) {
      throw bad_variant_access("invalid index");
    }
    return std::move(v.storage_.template get<Index>());
  }

  template <std::size_t Index>
    requires (Index < sizeof...(Types))
  friend constexpr const variant_alternative_t<Index, variant>& get(const variant& v) {
    if (v.index() != Index) {
      throw bad_variant_access("invalid index");
    }
    return v.storage_.template get<Index>();
  }

  template <std::size_t Index>
    requires (Index < sizeof...(Types))
  friend constexpr const variant_alternative_t<Index, variant>&& get(const variant&& v) {
    if (v.index() != Index) {
      throw bad_variant_access("invalid index");
    }
    return std::move(v.storage_.template get<Index>());
  }

  template <typename T>
    requires (exectly_one<T>)
  friend constexpr T& get(variant& v) {
    return get<type_to_index_v<T>>(v);
  }

  template <typename T>
    requires (exectly_one<T>)
  friend constexpr T&& get(variant&& v) {
    return get<type_to_index_v<T>>(std::move(v));
  }

  template <typename T>
    requires (exectly_one<T>)
  friend constexpr const T& get(const variant& v) {
    return get<type_to_index_v<T>>(v);
  }

  template <typename T>
    requires (exectly_one<T>)
  friend constexpr const T&& get(const variant&& v) {
    return get<type_to_index_v<T>>(std::move(v));
  }

  template <size_t I>
    requires (I < sizeof...(Types))
  friend constexpr std::add_pointer_t<variant_alternative_t<I, variant>> get_if(variant* pv) noexcept {
    if (pv != nullptr && pv->index() == I) {
      return std::addressof(pv->storage_.template get<I>());
    } else {
      return nullptr;
    }
  }

  template <size_t I>
    requires (I < sizeof...(Types))
  friend constexpr std::add_pointer_t<const variant_alternative_t<I, variant>> get_if(const variant* pv) noexcept {
    if (pv != nullptr && pv->index() == I) {
      return std::addressof(pv->storage_.template get<I>());
    } else {
      return nullptr;
    }
  }

  template <class T>
    requires (exectly_one<T>)
  friend constexpr std::add_pointer_t<T> get_if(variant* pv) noexcept {
    return get_if<type_to_index_v<T>>(pv);
  }

  template <class T>
    requires (exectly_one<T>)
  friend constexpr std::add_pointer_t<const T> get_if(const variant* pv) noexcept {
    return get_if<type_to_index_v<T>>(pv);
  }

  template <typename T>
    requires (exectly_one<T>)
  friend constexpr bool holds_alternative(const variant& v) noexcept {
    return v.alternative == type_to_index_v<T>;
  }

  friend constexpr bool operator==(const variant& v, const variant& w) {
    if (v.index() != w.index()) {
      return false;
    } else if (v.valueless_by_exception()) {
      return true;
    } else {
      return visit<bool>(
          detail::apply_lambda<bool, decltype([](const auto& a, const auto& b) -> bool { return a == b; })>(),
          v,
          w
      );
    }
  }

  friend constexpr bool operator!=(const variant& v, const variant& w) {
    if (v.index() != w.index()) {
      return true;
    } else if (v.valueless_by_exception()) {
      return false;
    } else {
      return visit<bool>(
          detail::apply_lambda<bool, decltype([](const auto& a, const auto& b) -> bool { return a != b; })>(),
          v,
          w
      );
    }
  }

  friend constexpr bool operator<(const variant& v, const variant& w) {
    if (w.valueless_by_exception()) {
      return false;
    } else if (v.valueless_by_exception()) {
      return true;
    } else if (v.index() < w.index()) {
      return true;
    } else if (v.index() > w.index()) {
      return false;
    } else {
      return visit<bool>(
          detail::apply_lambda<bool, decltype([](const auto& a, const auto& b) -> bool { return a < b; })>(),
          v,
          w
      );
    }
  }

  friend constexpr bool operator>(const variant& v, const variant& w) {
    if (v.valueless_by_exception()) {
      return false;
    } else if (w.valueless_by_exception()) {
      return true;
    } else if (v.index() > w.index()) {
      return true;
    } else if (v.index() < w.index()) {
      return false;
    } else {
      return visit<bool>(
          detail::apply_lambda<bool, decltype([](const auto& a, const auto& b) -> bool { return a > b; })>(),
          v,
          w
      );
    }
  }

  friend constexpr bool operator<=(const variant& v, const variant& w) {
    if (v.valueless_by_exception()) {
      return true;
    } else if (w.valueless_by_exception()) {
      return false;
    } else if (v.index() < w.index()) {
      return true;
    } else if (v.index() > w.index()) {
      return false;
    } else {
      return visit<bool>(
          detail::apply_lambda<bool, decltype([](const auto& a, const auto& b) -> bool { return a <= b; })>(),
          v,
          w
      );
    }
  }

  friend constexpr bool operator>=(const variant& v, const variant& w) {
    if (w.valueless_by_exception()) {
      return true;
    } else if (v.valueless_by_exception()) {
      return false;
    } else if (v.index() > w.index()) {
      return true;
    } else if (v.index() < w.index()) {
      return false;
    } else {
      return visit<bool>(
          detail::apply_lambda<bool, decltype([](const auto& a, const auto& b) -> bool { return a >= b; })>(),
          v,
          w
      );
    }
  }

  template <class... UTypes>
  friend constexpr std::common_comparison_category_t<std::compare_three_way_result_t<UTypes>...>
  operator<=>(const variant<UTypes...>& v, const variant<UTypes...>& w)
    requires std::is_same_v<variant<UTypes...>, variant> && (std::three_way_comparable<Types> && ...)
  {
    if (v.valueless_by_exception() && w.valueless_by_exception()) {
      return std::strong_ordering::equal;
    } else if (v.valueless_by_exception()) {
      return std::strong_ordering::less;
    } else if (w.valueless_by_exception()) {
      return std::strong_ordering::greater;
    } else if (v.index() != w.index()) {
      return v.index() <=> w.index();
    } else {
      using result_type = std::common_comparison_category_t<std::compare_three_way_result_t<Types>...>;
      return visit<result_type>(
          detail::apply_lambda<result_type, decltype([](const auto& a, const auto& b) -> result_type {
                                 return a <=> b;
                               })>(),
          v,
          w
      );
    }
  }

  friend constexpr void swap(variant& l, variant& r) noexcept(noexcept(l.swap(r)))
    requires (is_all_v<std::is_move_constructible> && is_all_v<std::is_swappable>)
  {
    l.swap(r);
  }

private:
  size_t alternative;
  detail::UnionStorage<Types...> storage_;
};

namespace detail {

template <size_t Pos, typename... Args>
class table_part;

template <size_t pos, typename Res, typename Visitor, typename... Variants>
  requires (pos == sizeof...(Variants))
class table_part<pos, Res, Visitor, Variants...> {
public:
  Res (*fn)(Visitor&&, Variants&&...);

  constexpr auto get(size_t*) const {
    return fn;
  }
};

template <size_t pos, typename Res, typename Visitor, typename... Variants>
class table_part<pos, Res, Visitor, Variants...> {
public:
  table_part<pos + 1, Res, Visitor, Variants...>
      arr[variant_size_v<std::remove_cvref_t<typename nth_in_pack<pos, Variants...>::type>>];

  constexpr auto get(size_t* poses) const {
    return arr[poses[pos]].get(poses);
  }
};

template <typename Res, size_t... nums, typename Visitor, typename... Variants>
constexpr Res visit_cell(Visitor&& vis, Variants&&... vars) {
  if constexpr (std::is_void_v<Res>) {
    return static_cast<void>(std::invoke(
        std::forward<Visitor>(vis),
        static_cast<get_qualifiers_t<decltype(vars.storage_.template get<nums>()), Variants&&>>(
            vars.storage_.template get<nums>()
        )...
    ));
  } else {
    return std::invoke(
        std::forward<Visitor>(vis),
        static_cast<get_qualifiers_t<decltype(vars.storage_.template get<nums>()), Variants&&>>(
            vars.storage_.template get<nums>()
        )...
    );
  }
}

template <typename... Args>
class visit_table;

template <typename Visitor, typename Res, typename... Variants>
class visit_table<Visitor, Res, Variants...> {
public:
  constexpr visit_table() noexcept {
    init_table<>(arr);
  }

  template <typename Vis, typename... Vars>
  constexpr auto visit(Vis&& vis, Vars&&... vars) const {
    size_t poses[sizeof...(Variants)] = {vars.index()...};
    return arr.get(poses)(std::forward<Vis>(vis), std::forward<Vars>(vars)...);
  }

private:
  template <size_t... nums, size_t... indexes, size_t level>
  constexpr void pass_indexes(std::index_sequence<indexes...>, table_part<level, Res, Visitor, Variants...>& storage) {
    (init_table<nums..., indexes>(storage.arr[indexes]), ...);
  }

  template <size_t... nums>
  constexpr void init_table(table_part<sizeof...(Variants), Res, Visitor, Variants...>& storage) {
    storage.fn = visit_cell<Res, nums...>;
  }

  template <size_t... nums, size_t level>
  constexpr void init_table(table_part<level, Res, Visitor, Variants...>& storage) {
    pass_indexes<nums...>(
        std::make_index_sequence<variant_size_v<std::remove_cvref_t<typename nth_in_pack<level, Variants...>::type>>>{},
        storage
    );
  }

private:
  table_part<0, Res, Visitor, Variants...> arr;
};

template <typename... Args>
inline static constexpr visit_table<Args...> VISIT_TABLE{};

} // namespace detail

template <typename Res, typename Visitor, typename... Types>
  requires (detail::visit_check_result_v<Visitor, Res, variant<>, detail::get_variant_t<Types>...>)
constexpr Res visit(Visitor&& vis, Types&&... vars) {
  if ((detail::asvariant(vars).valueless_by_exception() || ...)) {
    throw bad_variant_access("variant is valueless");
  }
  return detail::VISIT_TABLE<decltype(std::forward<Visitor>(vis)), Res, decltype(std::forward<detail::tovariant<Types>>(vars))...>.visit(
      std::forward<Visitor>(vis),
      std::forward<detail::tovariant<Types>>(vars)...
  );
}

template <
    typename Visitor,
    typename... Types,
    typename Res =
        std::invoke_result_t<Visitor, detail::get_first_v<detail::get_variant_t<detail::tovariant<Types>>>...>>
  requires (detail::visit_check_valid_v<Visitor, variant<>, detail::get_variant_t<detail::tovariant<Types>>...> && detail::visit_check_same_result_v<Visitor, Res, variant<>, detail::get_variant_t<detail::tovariant<Types>>...>)
constexpr Res visit(Visitor&& vis, Types&&... vars) {
  return visit<Res>(std::forward<Visitor>(vis), std::forward<Types>(vars)...);
}

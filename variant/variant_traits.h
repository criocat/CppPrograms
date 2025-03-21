#pragma once

#include <cstddef>
#include <exception>
#include <functional>
#include <memory>
#include <stdexcept>
#include <utility>

inline constexpr size_t variant_npos = (~static_cast<size_t>(0));

struct monostate {};

template <typename T>
struct in_place_type_t {
  explicit in_place_type_t() = default;
};

template <size_t I>
struct in_place_index_t {
  explicit in_place_index_t() = default;
};

template <size_t I>
inline constexpr in_place_index_t<I> in_place_index{};

template <typename T>
inline constexpr in_place_type_t<T> in_place_type{};

template <typename... Types>
class variant;

template <typename V>
struct variant_size;

template <typename... Types>
struct variant_size<variant<Types...>> : std::integral_constant<size_t, sizeof...(Types)> {};

template <typename V>
struct variant_size<const V> : variant_size<V> {};

template <typename V>
struct variant_size<volatile V> : variant_size<V> {};

template <typename V>
struct variant_size<const volatile V> : variant_size<V> {};

template <typename V>
static constexpr size_t variant_size_v = variant_size<V>::value;

class bad_variant_access : public std::exception {
public:
  bad_variant_access() noexcept
      : message_("variant wrong access") {}

  bad_variant_access(const bad_variant_access& other) noexcept = default;

  explicit bad_variant_access(const char* message) noexcept
      : message_(message) {}

  bad_variant_access& operator=(const bad_variant_access& other) noexcept = default;

  const char* what() const noexcept override {
    return message_;
  }

private:
  const char* message_;
};

namespace detail {

template <class... Ts>
constexpr auto&& asvariant(variant<Ts...>& var) {
  return var;
}

template <class... Ts>
constexpr auto&& asvariant(const variant<Ts...>& var) {
  return var;
}

template <class... Ts>
constexpr auto&& asvariant(variant<Ts...>&& var) {
  return std::move(var);
}

template <class... Ts>
constexpr auto&& asvariant(const variant<Ts...>&& var) {
  return std::move(var);
}

template <size_t, typename...>
struct nth_in_pack;

template <typename T, typename... Tail>
struct nth_in_pack<0, T, Tail...> {
  using type = T;
};

template <size_t Index, typename T, typename... Tail>
struct nth_in_pack<Index, T, Tail...> {
  using type = nth_in_pack<Index - 1, Tail...>::type;
};

template <typename T, typename... Args>
struct type_to_index {};

template <typename T, typename U, typename... Args>
struct type_to_index<T, U, Args...> {
  static constexpr size_t value = type_to_index<T, Args...>::value + 1;
};

template <typename T, typename... Args>
struct type_to_index<T, T, Args...> {
  static constexpr size_t value = 0;
};

template <typename Ec, typename... Args>
struct get_count;

template <typename Ec>
struct get_count<Ec> {
  static constexpr size_t value = 0;
};

template <typename Ec, typename T, typename... Tail>
struct get_count<Ec, T, Tail...> {
  static constexpr size_t value = get_count<Ec, Tail...>::value + (std::is_same_v<T, Ec> ? 1 : 0);
};

template <typename T>
struct Arr {
  T arr[1];
};

template <typename T, typename Ti>
concept check_one_func_valid = requires { Arr<Ti>({std::declval<T>()}); };

template <typename T, size_t Ind, typename Ti>
struct one_func_check {
  static constexpr std::integral_constant<size_t, Ind> func(Ti)
    requires (check_one_func_valid<T, Ti>);
  static constexpr void func()
    requires (!check_one_func_valid<T, Ti>);
};

template <typename Ind_seq, typename... Args>
struct func_check;

template <typename T, typename... Types, size_t... Inds>
struct func_check<T, std::index_sequence<Inds...>, Types...> : one_func_check<T, Inds, Types>... {
  using one_func_check<T, Inds, Types>::func...;
};

template <typename>
struct is_in_place_index {
  static constexpr bool value = false;
};

template <size_t Ind>
struct is_in_place_index<in_place_index_t<Ind>> {
  static constexpr bool value = true;
};

template <typename T>
static constexpr bool is_in_place_index_v = is_in_place_index<T>::value;

template <typename>
struct is_in_place_type {
  static constexpr bool value = false;
};

template <typename T>
struct is_in_place_type<in_place_type_t<T>> {
  static constexpr bool value = true;
};

template <typename T>
static constexpr bool is_in_place_type_v = is_in_place_type<T>::value;

template <typename T>
static constexpr bool not_in_place = !is_in_place_type_v<T> && !is_in_place_index_v<T>;

template <typename V>
struct get_variant;

template <typename... Types>
struct get_variant<variant<Types...>> {
  using type = variant<Types&&...>;
};

template <typename... Types>
struct get_variant<variant<Types...>&> {
  using type = variant<Types&...>;
};

template <typename... Types>
struct get_variant<variant<Types...>&&> {
  using type = variant<Types&&...>;
};

template <typename... Types>
struct get_variant<const variant<Types...>> {
  using type = variant<const Types&&...>;
};

template <typename... Types>
struct get_variant<const variant<Types...>&> {
  using type = variant<const Types&...>;
};

template <typename... Types>
struct get_variant<const variant<Types...>&&> {
  using type = variant<const Types&&...>;
};

template <typename V>
using get_variant_t = get_variant<V>::type;

template <template <typename> typename Pred, typename... Types>
struct is_all;

template <template <typename> typename Pred>
struct is_all<Pred> {
  static constexpr bool value = true;
};

template <template <typename> typename Pred, typename T, typename... Types>
struct is_all<Pred, T, Types...> {
  static constexpr bool value = is_all<Pred, Types...>::value && Pred<T>::value;
};

template <typename Visitor, typename Chosed, typename... Variants>
struct visit_check_valid;

template <typename... Args>
static constexpr bool visit_check_valid_v = visit_check_valid<Args...>::value;

template <typename Visitor, typename... Types>
struct visit_check_valid<Visitor, variant<Types...>> {
  static constexpr bool value = std::is_invocable_v<Visitor, Types...>;
};

template <typename Visitor, typename... Types, typename... VarTypes, typename... Variants>
struct visit_check_valid<Visitor, variant<Types...>, variant<VarTypes...>, Variants...> {
  static constexpr bool value = (visit_check_valid_v<Visitor, variant<Types..., VarTypes>, Variants...> && ...);
};

template <typename V>
struct get_first;

template <typename T, typename... Types>
struct get_first<variant<T, Types...>> {
  using type = T;
};

template <typename V>
using get_first_v = get_first<V>::type;

template <typename Visitor, typename Res, typename Chosed, typename... Variants>
struct visit_check_result;

template <typename... Args>
static constexpr bool visit_check_result_v = visit_check_result<Args...>::value;

template <typename Visitor, typename Res, typename... Types>
struct visit_check_result<Visitor, Res, variant<Types...>> {
  static constexpr bool value = std::is_invocable_r_v<Res, Visitor, Types...>;
};

template <typename Visitor, typename Res, typename... Types, typename... VarTypes, typename... Variants>
struct visit_check_result<Visitor, Res, variant<Types...>, variant<VarTypes...>, Variants...> {
  static constexpr bool value = (visit_check_result_v<Visitor, Res, variant<Types..., VarTypes>, Variants...> && ...);
};

template <typename Visitor, typename Res, typename Chosed, typename... Variants>
struct visit_check_same_result;

template <typename... Args>
static constexpr bool visit_check_same_result_v = visit_check_same_result<Args...>::value;

template <typename Visitor, typename Res, typename... Types>
struct visit_check_same_result<Visitor, Res, variant<Types...>> {
  static constexpr bool value = std::is_same_v<std::invoke_result_t<Visitor, Types...>, Res>;
};

template <typename Visitor, typename Res, typename... Types, typename... VarTypes, typename... Variants>
struct visit_check_same_result<Visitor, Res, variant<Types...>, variant<VarTypes...>, Variants...> {
  static constexpr bool value =
      (visit_check_same_result_v<Visitor, Res, variant<Types..., VarTypes>, Variants...> && ...);
};

template <typename T>
using tovariant = decltype(detail::asvariant(std::declval<T>()));

template <typename T, typename... Types>
concept check_conversion =
    requires { func_check<T, std::make_index_sequence<sizeof...(Types)>, Types...>::func(std::declval<T>()); };

template <typename T>
constexpr T& remove_ref_const(const T& value) {
  return const_cast<T&>(value);
}

template <typename T>
constexpr T& remove_ref_const(T& value) {
  return const_cast<T&>(value);
}

template <typename T, typename V>
struct get_qualifiers;

template <typename... Args, typename T>
struct get_qualifiers<T, variant<Args...>&> {
  using type = std::remove_reference_t<T>&;
};

template <typename... Args, typename T>
struct get_qualifiers<T, const variant<Args...>&> {
  using type = const std::remove_reference_t<T>&;
};

template <typename... Args, typename T>
struct get_qualifiers<T, variant<Args...>&&> {
  using type = std::remove_reference_t<T>&&;
};

template <typename... Args, typename T>
struct get_qualifiers<T, const variant<Args...>&&> {
  using type = const std::remove_reference_t<T>&&;
};

template <typename T, typename V>
using get_qualifiers_t = get_qualifiers<T, V>::type;

} // namespace detail

template <size_t I, typename V>
struct variant_alternative;

template <size_t I, typename... Types>
struct variant_alternative<I, variant<Types...>> {
  using type = detail::nth_in_pack<I, Types...>::type;
};

template <size_t I, typename... Types>
struct variant_alternative<I, const variant<Types...>> {
  using type = const detail::nth_in_pack<I, Types...>::type;
};

template <size_t I, typename... Types>
struct variant_alternative<I, volatile variant<Types...>> {
  using type = volatile detail::nth_in_pack<I, Types...>::type;
};

template <size_t I, typename... Types>
struct variant_alternative<I, const volatile variant<Types...>> {
  using type = const volatile detail::nth_in_pack<I, Types...>::type;
};

template <size_t I, class T>
using variant_alternative_t = variant_alternative<I, T>::type;

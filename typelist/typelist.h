#pragma once

#include <cstddef>
#include <type_traits>
#include <utility>

namespace tl {

template <typename...>
struct type_list {};

// Transform
template <template <typename...> typename F, typename List>
struct transform_impl;

template <template <typename...> typename F, typename... Types, template <typename...> typename List>
struct transform_impl<F, List<Types...>> {
  using type = List<F<Types>...>;
};

template <template <typename...> typename F, typename List>
using transform = typename transform_impl<F, List>::type;

// Apply
template <template <typename...> typename F, typename List>
struct apply_impl;

template <template <typename...> typename F, template <typename...> typename List, typename... Types>
struct apply_impl<F, List<Types...>> {
  using type = F<Types...>;
};

template <template <typename...> typename F, typename List>
using apply = typename apply_impl<F, List>::type;

// reverse2
template <typename List>
struct reverse2_impl;

template <typename T1, typename T2, template <typename...> typename List>
struct reverse2_impl<List<T1, T2>> {
  using type = List<T2, T1>;
};

template <typename List>
using reverse2 = typename reverse2_impl<List>::type;

// flip
template <typename List>
using flip = transform<reverse2, List>;

// concat
template <typename Type>
struct identity {
  using type = Type;
};

template <
    template <typename...>
    typename List1,
    template <typename...>
    typename List2,
    typename... Types1,
    typename... Types2>
identity<List1<Types1..., Types2...>> operator+(identity<List1<Types1...>>, identity<List2<Types2...>>);

template <typename... Lists>
using concat = typename decltype((identity<Lists>{} + ... + identity<type_list<>>{}))::type;

// change_list
template <typename List, template <typename...> typename NewList>
struct change_list_impl;

template <template <typename...> typename NewList, template <typename...> typename CurList, typename... Values>
struct change_list_impl<CurList<Values...>, NewList> {
  using type = NewList<Values...>;
};

template <typename List, template <typename...> typename NewList>
using change_list = typename change_list_impl<List, NewList>::type;

// flatten
template <typename Lists>
struct flatten_impl;

template <typename List>
using flatten = typename flatten_impl<List>::type;

template <typename T>
struct flatten_impl {
  using type = type_list<T>;
};

template <typename... Lists, template <typename...> typename CurList>
struct flatten_impl<CurList<Lists...>> {
  using temp = transform<flatten, CurList<Lists...>>;
  using type = change_list<apply<concat, temp>, CurList>;
};

// Map Find
template <typename... Bases>
struct inherit : Bases... {};

template <typename Key, typename Map>
struct map_find_impl;

template <typename Key, typename... Lists, template <typename...> typename List>
struct map_find_impl<Key, List<Lists...>> {
  template <typename... Rest>
  static type_list<Key, Rest...> f(type_list<Key, Rest...>*);

  using derived = inherit<Lists...>;

  using type = decltype(f(static_cast<derived*>(nullptr)));
};

template <typename Key, typename Map>
using map_find = typename map_find_impl<Key, Map>::type;

// Count
template <typename List>
struct count_impl;

template <typename... Types, template <typename...> typename List>
struct count_impl<List<Types...>> {
  static constexpr size_t value = sizeof...(Types);
};

template <typename List>
inline constexpr size_t count = count_impl<List>::value;

// Index
template <size_t N>
using index = std::integral_constant<std::size_t, N>;

// Enumerate
template <typename List, typename Seq>
struct enumerate_impl;

template <typename... Types, size_t... Idxs, template <typename...> typename List>
struct enumerate_impl<List<Types...>, std::index_sequence<Idxs...>> {
  using type = List<type_list<index<Idxs>, Types>...>;
};

template <typename List>
using enumerate = typename enumerate_impl<List, std::make_index_sequence<count<List>>>::type;

// Index_of_unique
template <typename List>
struct index_of_unique_value;

template <typename Type, size_t N>
struct index_of_unique_value<type_list<Type, index<N>>> {
  static constexpr size_t value = N;
};

template <typename Type, typename List>
struct index_of_unique_impl {
  using temp = map_find<Type, flip<enumerate<List>>>;
  static constexpr size_t value = index_of_unique_value<temp>::value;
};

template <typename Key, typename List>
inline constexpr size_t index_of_unique = index_of_unique_impl<Key, List>::value;

// Push_front
template <typename Type, typename List>
struct push_front_impl;

template <typename Type, template <typename...> typename List, typename... Values>
struct push_front_impl<Type, List<Values...>> {
  using type = List<Type, Values...>;
};

template <typename Type, typename List>
using push_front = typename push_front_impl<Type, List>::type;

// get_second

template <typename Pair>
struct get_second_impl;

template <typename Ind, typename Val, template <typename...> typename List>
struct get_second_impl<List<Ind, Val>> {
  using type = Val;
};

template <typename Pair>
using get_second = typename get_second_impl<Pair>::type;

// Filter
template <template <typename...> typename Pred, typename List>
struct filter_impl;

template <template <typename...> typename Pred, typename List>
using filter = typename filter_impl<Pred, List>::type;

template <template <typename...> typename Pred, typename... Types, template <typename...> typename List>
struct filter_impl<Pred, List<Types...>> {
  template <typename Type>
  using single = typename std::conditional_t<Pred<Type>::value, List<Type>, List<>>;
  using type = concat<single<Types>...>;
};

template <template <typename...> typename Pred, template <typename...> typename List>
struct filter_impl<Pred, List<>> {
  using type = List<>;
};

// Merge
template <template <typename...> typename Comp, typename List1, typename List2>
struct merge_impl;

template <template <typename...> typename Comp, typename List1, typename List2>
using merge = typename merge_impl<Comp, List1, List2>::type;

template <template <typename...> typename Comp, template <typename...> typename List>
struct merge_impl<Comp, List<>, List<>> {
  using type = List<>;
};

template <template <typename...> typename Comp, template <typename...> typename List, typename... Values>
struct merge_impl<Comp, List<>, List<Values...>> {
  using type = List<Values...>;
};

template <template <typename...> typename Comp, template <typename...> typename List, typename... Values>
struct merge_impl<Comp, List<Values...>, List<>> {
  using type = List<Values...>;
};

template <
    template <typename...>
    typename Comp,
    template <typename...>
    typename List,
    typename... ValuesL,
    typename... ValuesR,
    typename ValL,
    typename ValR>
struct merge_impl<Comp, List<ValL, ValuesL...>, List<ValR, ValuesR...>> {
  using ListL = std::conditional_t<Comp<ValL, ValR>::value, List<ValuesL...>, List<ValL, ValuesL...>>;
  using ListR = std::conditional_t<Comp<ValL, ValR>::value, List<ValR, ValuesR...>, List<ValuesR...>>;
  using rest = merge<Comp, ListL, ListR>;
  using type = push_front<std::conditional_t<Comp<ValL, ValR>::value, ValL, ValR>, rest>;
};

// Merge_sort
template <template <typename...> typename Comp, typename List>
struct merge_sort_impl;

template <typename List, template <typename...> typename Comp>
using merge_sort = typename merge_sort_impl<Comp, List>::type;

template <template <typename...> typename Comp, template <typename...> typename List>
struct merge_sort_impl<Comp, List<>> {
  using type = List<>;
};

template <template <typename...> typename Comp, template <typename...> typename List, typename Val>
struct merge_sort_impl<Comp, List<Val>> {
  using type = List<Val>;
};

template <template <typename...> typename Comp, template <typename...> typename List, typename... Values>
struct merge_sort_impl<Comp, List<Values...>> {
  static constexpr size_t cnt = count<List<Values...>>;

  template <typename T>
  struct less_than_impl;

  template <size_t N, typename Type>
  struct less_than_impl<type_list<index<N>, Type>> {
    static constexpr bool value = (N < cnt / 2);
  };

  template <typename Pair>
  struct more_than_impl {
    static constexpr bool value = !less_than_impl<Pair>::value;
  };

  using LList = transform<get_second, filter<less_than_impl, enumerate<List<Values...>>>>;
  using RList = transform<get_second, filter<more_than_impl, enumerate<List<Values...>>>>;
  using SortLList = merge_sort<LList, Comp>;
  using SortRList = merge_sort<RList, Comp>;
  using type = merge<Comp, SortLList, SortRList>;
};

// Contains
template <typename Type, typename List>
struct contains_impl;

template <typename Type, typename List>
struct contains_impl {
  template <typename Val>
  struct Pred {
    static const bool value = std::is_same_v<Type, Val>;
  };

  static const bool value = count<filter<Pred, List>> > 0;
};

template <typename Type, typename List>
static const bool contains = contains_impl<Type, List>::value;
} // namespace tl

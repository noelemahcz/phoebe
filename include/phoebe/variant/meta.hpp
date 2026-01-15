#pragma once


#include <type_traits>


namespace meta {


// template <template <typename...> typename Trait>
// struct quote {
//   template <typename... Ts>
//   using invoke = Trait<Ts...>;
// };
//
//
// struct comp {
//   template <typename F, typename G>
//   struct invoke {
//     struct type {
//       template <typename T>
//       using invoke = F::template invoke<G::template invoke<T>::type>;
//     };
//   };
// };


// TODO: `has_type_member` & `has_value_member<T>`


template </*has_value_member<bool>*/ typename Pred, typename T, typename U>
struct select : std::conditional<Pred::value, T, U> {};

template </*has_value_member<bool>*/ typename Pred, typename T, typename U>
using select_t = typename select<Pred, T, U>::type;


template <template <typename...> typename F, template <typename...> typename G>
struct compose {
  template <typename... Ts>
  using type = F<typename G<Ts...>::type>;
};


// List utilities


template <typename... Ts>
struct list;


template <typename List>
inline constexpr bool empty_v = false;

template <template <typename...> class List>
inline constexpr bool empty_v<List<>> = true;

template <typename List>
struct empty : std::bool_constant<empty_v<List>> {};


template <template <typename...> typename F, typename List>
struct apply {};

template <template <typename...> typename F, template <typename...> typename List, typename... Ts>
struct apply<F, List<Ts...>> {
  using type = F<Ts...>;
};

template <template <typename...> typename F, typename List>
using apply_t = typename apply<F, List>::type;


template <template <typename> typename F, typename List>
struct map {};

template <template <typename> typename F, template <typename...> typename List, typename... Ts>
struct map<F, List<Ts...>> {
  using type = List<F<Ts>...>;
};

template <template <typename> typename F, typename List>
using map_t = typename map<F, List>::type;


template </*list_of_has_value_member<bool>*/ typename List>
struct all {};

// NOTE: Lazy evaluation.
template <template <typename...> typename List, /*has_value_member<bool>...*/ typename... Preds>
struct all<List<Preds...>> : std::conjunction<Preds...> {};

// NOTE: Eager evaluation.
//
// namespace detail {
// template <bool... Preds>
// struct all_dummy;
// } // namespace detail
//
// template <template <typename...> typename List, /*list_of_has_value_member<bool>*/ typename... Preds>
// struct all<List<Preds...>>
//     : std::is_same<detail::all_dummy<Preds::value...>, detail::all_dummy<((void)Preds::value, true)...>> {};

template </*list_of_has_value_member<bool>*/ typename List>
inline constexpr bool all_v = all<List>::value;


template </*list_of_has_value_member<bool>*/ typename List>
struct any {};

// NOTE: Lazy evaluation.
template <template <typename...> typename List, /*has_value_member<bool>...*/ typename... Preds>
struct any<List<Preds...>> : std::disjunction<Preds...> {};

template </*list_of_has_value_member<bool>*/ typename List>
inline constexpr bool any_v = any<List>::value;


// concat :: [[a]] -> [a]
// Concatenate a list of lists.
template <typename List, typename... Rest>
struct concat {
  using type = List;
};

template <template <typename...> typename List, typename... Ts, typename... Us>
struct concat<List<Ts...>, List<Us...>> {
  using type = List<Ts..., Us...>;
};

template <template <typename...> typename List, typename... Ts, typename... Us, typename... Rest>
struct concat<List<Ts...>, List<Us...>, Rest...> : concat<List<Ts..., Us...>, Rest...> {};

template <typename List, typename... Rest>
using concat_t = typename concat<List, Rest...>::type;


// FIXME: Lazy?
// filter :: (a -> Bool) -> [a] -> [a]
// Applied to a predicate and a list, returns the list of those elements that satisfy the predicate.
template <template <typename> typename Pred, typename List>
struct filter {};

template <template <typename> typename Pred, template <typename...> typename List>
struct filter<Pred, List<>> {
  using type = List<>;
};

template <template <typename> typename Pred, template <typename...> typename List, typename... Ts>
struct filter<Pred, List<Ts...>> : concat<select_t<Pred<Ts>, List<Ts>, List<>>...> {};

template <template <typename> typename Pred, typename List>
using filter_t = typename filter<Pred, List>::type;


} // namespace meta

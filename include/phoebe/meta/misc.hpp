#pragma once


#include <type_traits>


namespace meta {


template <typename T, typename U>
struct forward_cvref {
private:
  using unref_t = std::remove_reference_t<T>;
  using unref_u = std::remove_reference_t<U>;

  using const_u = std::conditional_t<std::is_const_v<unref_t>, unref_u const, unref_u>;
  using cv_u = std::conditional_t<std::is_volatile_v<unref_t>, const_u volatile, const_u>;

public:
  using type = std::conditional_t<std::is_lvalue_reference_v<T>, cv_u&, cv_u&&>;
};

template <typename T, typename U>
using forward_cvref_t = forward_cvref<T, U>::type;


template <template <typename...> typename F, typename... BoundArgs>
struct bind {
  template <typename... CallArgs>
  using type = F<BoundArgs..., CallArgs...>;
};

template <template <typename...> typename F, typename... BoundArgs>
using bind_t = bind<F, BoundArgs...>::type;


template <template <typename...> typename F, typename... BoundArgs>
struct bind_back {
  template <typename... CallArgs>
  using type = F<CallArgs..., BoundArgs...>;
};

template <template <typename...> typename F, typename... BoundArgs>
using bind_back_t = bind_back<F, BoundArgs...>::type;


} // namespace meta

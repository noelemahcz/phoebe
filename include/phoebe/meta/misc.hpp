#pragma once


namespace meta {


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

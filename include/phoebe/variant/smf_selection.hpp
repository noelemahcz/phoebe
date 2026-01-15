#pragma once


#include <type_traits>


namespace phoebe::detail::smf_selection {


template <typename Base>
struct non_trivial_copy_ctor : Base {
  using Base::Base;
  non_trivial_copy_ctor() = default;
  non_trivial_copy_ctor(non_trivial_copy_ctor const&) /*noexcept*/ {
    // TODO: IMPL
  }
  non_trivial_copy_ctor(non_trivial_copy_ctor&&) = default;
  non_trivial_copy_ctor& operator=(non_trivial_copy_ctor const&) = default;
  non_trivial_copy_ctor& operator=(non_trivial_copy_ctor&&) = default;
};

template <typename Base>
struct deleted_copy_ctor : Base {
  using Base::Base;
  deleted_copy_ctor() = default;
  deleted_copy_ctor(deleted_copy_ctor const&) = delete;
  deleted_copy_ctor(deleted_copy_ctor&&) = default;
  deleted_copy_ctor& operator=(deleted_copy_ctor const&) = default;
  deleted_copy_ctor& operator=(deleted_copy_ctor&&) = default;
};

template <typename Base, typename... Ts>
using select_copy_ctor = std::conditional_t<std::conjunction_v<std::is_trivially_copy_constructible<Ts>...>, Base,
                                            std::conditional_t<std::conjunction_v<std::is_copy_constructible<Ts>...>,
                                                               non_trivial_copy_ctor<Base>, deleted_copy_ctor<Base>>>;


template <typename Base, typename... Ts>
struct non_trivial_move_ctor : select_copy_ctor<Base, Ts...> {
  using base = select_copy_ctor<Base, Ts...>;
  using base::base;
  non_trivial_move_ctor() = default;
  non_trivial_move_ctor(non_trivial_move_ctor const&) = default;
  non_trivial_move_ctor(non_trivial_move_ctor&&) /*noexcept*/ {
    // TODO: IMPL
  }
  non_trivial_move_ctor& operator=(non_trivial_move_ctor const&) = default;
  non_trivial_move_ctor& operator=(non_trivial_move_ctor&&) = default;
};

template <typename Base, typename... Ts>
struct deleted_move_ctor : select_copy_ctor<Base, Ts...> {
  using base = select_copy_ctor<Base, Ts...>;
  using base::base;
  deleted_move_ctor() = default;
  deleted_move_ctor(deleted_move_ctor const&) = default;
  deleted_move_ctor(deleted_move_ctor&&) = delete;
  deleted_move_ctor& operator=(deleted_move_ctor const&) = default;
  deleted_move_ctor& operator=(deleted_move_ctor&&) = default;
};

template <typename Base, typename... Ts>
using select_move_ctor =
    std::conditional_t<std::conjunction_v<std::is_trivially_move_constructible<Ts>...>, select_copy_ctor<Base, Ts...>,
                       std::conditional_t<std::conjunction_v<std::is_move_constructible<Ts>...>,
                                          non_trivial_move_ctor<Base, Ts...>, deleted_move_ctor<Base, Ts...>>>;


template <typename Base, typename... Ts>
struct non_trivial_copy_assign : select_move_ctor<Base, Ts...> {
  using base = select_move_ctor<Base, Ts...>;
  using base::base;
  non_trivial_copy_assign() = default;
  non_trivial_copy_assign(non_trivial_copy_assign const&) = default;
  non_trivial_copy_assign(non_trivial_copy_assign&&) = default;
  non_trivial_copy_assign& operator=(non_trivial_copy_assign const&) /*noexcept*/ {
    // TODO: IMPL
    return *this;
  }
  non_trivial_copy_assign& operator=(non_trivial_copy_assign&&) = default;
};

template <typename Base, typename... Ts>
struct deleted_copy_assign : select_move_ctor<Base, Ts...> {
  using base = select_move_ctor<Base, Ts...>;
  using base::base;
  deleted_copy_assign() = default;
  deleted_copy_assign(deleted_copy_assign const&) = default;
  deleted_copy_assign(deleted_copy_assign&&) = default;
  deleted_copy_assign& operator=(deleted_copy_assign const&) = delete;
  deleted_copy_assign& operator=(deleted_copy_assign&&) = default;
};

template <typename Base, typename... Ts>
using select_copy_assign = std::conditional_t<
    std::conjunction_v<std::is_trivially_copy_constructible<Ts>..., std::is_trivially_copy_assignable<Ts>...>,
    select_move_ctor<Base, Ts...>,
    std::conditional_t<std::conjunction_v<std::is_copy_constructible<Ts>..., std::is_copy_assignable<Ts>...>,
                       non_trivial_copy_assign<Base, Ts...>, deleted_copy_assign<Base, Ts...>>>;


template <typename Base, typename... Ts>
struct non_trivial_move_assign : select_copy_assign<Base, Ts...> {
  using base = select_copy_assign<Base, Ts...>;
  using base::base;
  non_trivial_move_assign() = default;
  non_trivial_move_assign(non_trivial_move_assign const&) = default;
  non_trivial_move_assign(non_trivial_move_assign&&) = default;
  non_trivial_move_assign& operator=(non_trivial_move_assign const&) = default;
  non_trivial_move_assign& operator=(non_trivial_move_assign&&) /*noexcept*/ {
    // TODO: IMPL
    return *this;
  }
};

template <typename Base, typename... Ts>
struct deleted_move_assign : select_copy_assign<Base, Ts...> {
  using base = select_copy_assign<Base, Ts...>;
  using base::base;
  deleted_move_assign() = default;
  deleted_move_assign(deleted_move_assign const&) = default;
  deleted_move_assign(deleted_move_assign&&) = default;
  deleted_move_assign& operator=(deleted_move_assign const&) = default;
  deleted_move_assign& operator=(deleted_move_assign&&) = delete;
};

template <typename Base, typename... Ts>
using select_move_assign = std::conditional_t<
    std::conjunction_v<std::is_trivially_move_constructible<Ts>..., std::is_trivially_move_assignable<Ts>...>,
    select_copy_assign<Base, Ts...>,
    std::conditional_t<std::conjunction_v<std::is_move_constructible<Ts>..., std::is_move_assignable<Ts>...>,
                       non_trivial_move_assign<Base, Ts...>, deleted_move_assign<Base, Ts...>>>;


template <typename Base, typename... Ts>
using select_smf = select_move_assign<Base, Ts...>;


} // namespace phoebe::detail::smf_selection

#ifndef VELOCYPACK_GADGETS_H
#define VELOCYPACK_GADGETS_H
#include <cstddef>
#include <tuple>

namespace deserializer::detail::gadgets {
namespace detail {

template<std::size_t I, typename...>
struct index_of_type;

template<std::size_t I, typename T, typename E, typename... Ts>
struct index_of_type<I, T, E, Ts...> {
  constexpr static auto value = index_of_type<I+1, T, Ts...>::value;
};

template<std::size_t I, typename T, typename... Ts>
struct index_of_type<I, T, T, Ts...> {
  constexpr static auto value = I;
};

}

template<typename T, typename... Ts>
using index_of_type = detail::index_of_type<0, T, Ts...>;
template<typename T, typename... Ts>
constexpr const auto index_of_type_v = detail::index_of_type<0, T, Ts...>::value;

template <class... Ts>
struct visitor : Ts... {
  using Ts::operator()...;
};
template <class... Ts>
visitor(Ts...)->visitor<Ts...>;

template <typename R, typename F, typename T>
struct is_applicable_r;

template <typename R, typename F, typename... Ts>
struct is_applicable_r<R, F, std::tuple<Ts...>> {
  constexpr static bool value = std::is_invocable_r_v<R, F, Ts...>;
};

template <typename T, typename C = void>
class is_complete_type : public std::false_type {};

template <typename T>
class is_complete_type<T, decltype(void(sizeof(T)))> : public std::true_type {};

template <typename T>
constexpr const bool is_complete_type_v = is_complete_type<T>::value;

namespace detail {
template <typename... Ts>
struct tuple_no_void_impl;

template <typename... Ts, typename E, typename... Es>
struct tuple_no_void_impl<std::tuple<Ts...>, E, Es...> {
  using type = typename tuple_no_void_impl<std::tuple<Ts..., E>, Es...>::type;
};

template <typename... Ts, typename... Es>
struct tuple_no_void_impl<std::tuple<Ts...>, void, Es...> {
  using type = typename tuple_no_void_impl<std::tuple<Ts...>, Es...>::type;
};

template <typename... Ts>
struct tuple_no_void_impl<std::tuple<Ts...>> {
  using type = std::tuple<Ts...>;
};
}  // namespace detail

template <typename... Ts>
struct tuple_no_void {
  using type = typename detail::tuple_no_void_impl<std::tuple<>, Ts...>::type;
};

}  // namespace deserializer::detail::gadgets

#endif  // VELOCYPACK_GADGETS_H

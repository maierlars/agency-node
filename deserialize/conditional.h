#ifndef VELOCYPACK_CONDITIONAL_H
#define VELOCYPACK_CONDITIONAL_H
#include "plan-executor.h"
#include "values.h"

namespace deserializer {

/*
 * Selects a deserializer depending on the first condition that evaluates to true.
 */
template <typename... VSs>
struct conditional;

template <typename C, typename S>
struct condition_deserializer_pair {
  using condition = C;
  using deserializer = S;
};

template <typename D>
struct conditional_default {
  using deserializer = D;
};

template <typename F, typename C = void>
class is_condition_deserializer_pair : public std::true_type {};
template <typename F>
class is_condition_deserializer_pair<F, std::void_t<typename F::condition, typename F::deserializer>>
    : public std::true_type {
  static_assert(is_deserializer_v<typename F::deserializer>);
};

template <typename F, typename C = void>
class is_conditional_default : public std::true_type {};
template <typename F>
class is_conditional_default<F, std::void_t<typename F::deserializer>>
    : public std::true_type {
  static_assert(is_deserializer_v<typename F::deserializer>);
};

template <typename F>
constexpr bool is_condition_deserializer_pair_v =
    is_condition_deserializer_pair<F>::value;
template <typename F>
constexpr bool is_conditional_default_v = is_conditional_default<F>::value;

template <typename... CSs>
struct conditional {
  using constructed_type = std::variant<typename CSs::deserializer::constructed_type...>;

  static_assert(sizeof...(CSs) > 0, "need at lease one alternative");

  static_assert(((is_condition_deserializer_pair_v<CSs> || is_conditional_default_v<CSs>)&&...),
                "list shall only contain `condition_deserializer_pair`s");
};

template <typename... VSs>
struct conditional_deserializer {
  using plan = conditional<VSs...>;
  using constructed_type = typename plan::constructed_type;
  using factory = utilities::identity_factory<constructed_type>;
};

struct is_object_condition {
  static bool test(::deserializer::slice_type s) noexcept {
    return s.isObject();
  }
};

namespace detail {

template <std::size_t I, typename...>
struct conditional_executor;

template <std::size_t I, typename E, typename D, typename... CDs>
struct conditional_executor<I, E, conditional_default<D>, CDs...> {
  using R = typename E::variant_type;
  using unpack_result = result<R, deserialize_error>;
  static auto unpack(::deserializer::slice_type s) -> unpack_result {
    static_assert(sizeof...(CDs) == 0, "conditional_default must be last");

    using namespace std::string_literals;
    return deserialize_with<D>(s).map([](auto const& v) {
      return R{std::in_place_index<I>, v};
    });
  }
};

template <std::size_t I, typename E, typename C, typename D, typename... CDs>
struct conditional_executor<I, E, condition_deserializer_pair<C, D>, CDs...> {
  using R = typename E::variant_type;
  using unpack_result = result<R, deserialize_error>;
  static auto unpack(::deserializer::slice_type s) -> unpack_result {
    using namespace std::string_literals;
    if (C::test(s)) {
      return deserialize_with<D>(s).map([](typename D::constructed_type&& v) {
        return R(std::in_place_index<I>, std::move(v));
      });
    }

    return conditional_executor<I + 1, E, CDs...>::unpack(s);
  }
};

template <std::size_t I, typename E>
struct conditional_executor<I, E> {
  using R = typename E::variant_type;
  using unpack_result = result<R, deserialize_error>;
  static auto unpack(::deserializer::slice_type s, ::deserializer::slice_type v)
      -> unpack_result {
    using namespace std::string_literals;
    return unpack_result{deserialize_error{"unrecognized value `"s + v.toJson() + "`"}};
  }
};
}  // namespace detail

namespace executor {
template <typename... CSs>
struct plan_result_tuple<conditional<CSs...>> {
  using variant = typename conditional<CSs...>::constructed_type;
  using type = std::tuple<variant>;
};

template <typename... CSs, typename H>
struct deserialize_plan_executor<conditional<CSs...>, H> {
  using executor_type = deserialize_plan_executor<conditional<CSs...>, H>;
  using plan_result_tuple_type = plan_result_tuple<conditional<CSs...>>;
  using unpack_tuple_type = typename plan_result_tuple_type::type;
  using variant_type = typename plan_result_tuple_type::variant;
  using unpack_result = result<unpack_tuple_type, deserialize_error>;

  static auto unpack(::deserializer::slice_type s, typename H::state_type hints)
      -> unpack_result {
    /*
     * Select the sub deserializer depending on the value.
     * Delegate to that deserializer.
     */
    using namespace std::string_literals;

    return ::deserializer::detail::conditional_executor<0, executor_type, CSs...>::unpack(s)
        .map([](variant_type&& v) { return std::make_tuple(std::move(v)); })
        .wrap([](deserialize_error&& e) {
          return std::move(e).wrap("when parsing conditionally");
        });
  }
};

}  // namespace executor
}  // namespace deserializer
#endif  // VELOCYPACK_CONDITIONAL_H
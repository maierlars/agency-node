#ifndef VELOCYPACK_FIXED_ORDER_H
#define VELOCYPACK_FIXED_ORDER_H
#include "deserialize-with.h"
#include "plan-executor.h"
#include "utilities.h"

namespace deserializer::fixed_order {

/*
 * Deserializes an array using a fixed order of deserializers. The result type
 * is a tuple containing the result types of deserializers in order they appear
 * in the template argument `Ds`.
 *
 * The factory is called with a single tuple parameter.
 */
template <typename... Ds>
struct fixed_order_deserializer {
  using plan = fixed_order_deserializer<Ds...>;
  using constructed_type = std::tuple<typename Ds::constructed_type...>;
  using factory = utilities::identity_factory<constructed_type>;
};

}  // namespace deserializer::fixed_order

namespace deserializer::executor {

namespace detail {

template <std::size_t I, typename T, typename E>
struct fixed_order_deserializer_executor_visitor {
  T& value_store;
  E& error_store;
  explicit fixed_order_deserializer_executor_visitor(T& value_store, E& error_store)
      : value_store(value_store), error_store(error_store) {}

  bool operator()(T t) {
    value_store = std::move(t);
    return true;
  }

  bool operator()(E e) {
    using namespace std::string_literals;
    error_store =
        std::move(e).wrap("in fixed order array at position "s + std::to_string(I));
    return false;
  }
};

}  // namespace detail

template <typename... Ds>
struct deserialize_plan_executor<fixed_order::fixed_order_deserializer<Ds...>> {
  using value_type = typename fixed_order::fixed_order_deserializer<Ds...>::constructed_type;
  using tuple_type = std::tuple<value_type>;
  using result_type = result<tuple_type, deserialize_error>;

  constexpr static auto expected_array_length = sizeof...(Ds);

  static auto unpack(::deserializer::slice_type s) -> result_type {
    using namespace std::string_literals;

    if (!s.isArray()) {
      return result_type{deserialize_error{"expected array"}};
    }

    if (s.length() != expected_array_length) {
      return result_type{deserialize_error{
          "bad array length, found: "s + std::to_string(s.length()) +
          ", expected: " + std::to_string(expected_array_length)}};
    }
    return unpack_internal(s, std::index_sequence_for<Ds...>{});
  }

 private:
  template <std::size_t... I>
  static auto unpack_internal(::deserializer::slice_type s, std::index_sequence<I...>)
      -> result_type {
    value_type values;
    deserialize_error error;

    bool result =
        (deserialize_with<Ds>(s.at(I)).visit(
             detail::fixed_order_deserializer_executor_visitor<I, typename Ds::constructed_type, deserialize_error>(
                 std::get<I>(values), error)) &&
         ...);

    if (result) {
      return result_type{values};
    }

    return result_type{error};
  }
};
}  // namespace deserializer::executor

#endif  // VELOCYPACK_FIXED_ORDER_H

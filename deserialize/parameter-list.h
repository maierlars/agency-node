#ifndef VELOCYPACK_PARAMETER_LIST_H
#define VELOCYPACK_PARAMETER_LIST_H
#include "gadgets.h"
#include "values.h"

namespace deserializer {

namespace parameter_list {

/*
 * parameter_list is used to specifiy a list of object fields that are decoded
 * and used as parameters to the factory. Possible types in `T` are
 * - factory_simple_parameter
 * - factory_slice_parameter
 * - expected_value
 */
template <typename... T>
struct parameter_list {
  constexpr static auto length = sizeof...(T);
};

/*
 * factory_simple_parameter describes a simple type parameter. `N` is the field
 * name, `T` the type of the parameter. If `required` is false,
 * `default_v::value` is used, otherwise the deserialization fails.
 */
template <char const N[], typename T, bool required, typename default_v = values::default_constructed_value<T>>
struct factory_simple_parameter {
  using value_type = T;
  constexpr static bool is_required = required;
  constexpr static auto name = N;
  constexpr static auto default_value = default_v::value;
};

/*
 * Same as factory_simple_parameter, except for Slices. The default value is the null slice.
 */

template <char const N[], bool required>
struct factory_slice_parameter {
  using value_type = arangodb::velocypack::Slice;
  constexpr static bool is_required = required;
  constexpr static auto name = N;
  constexpr static auto default_value = arangodb::velocypack::Slice::nullSlice();
};

/*
 * expected_value does not generate a additional parameter to the factory but instead
 * checks if the field with name `N` has the expected value `V`. If not, deserialization fails.
 */

template <const char N[], typename V>
struct expected_value {
  using value_type = void;
  using value = V;
  constexpr static bool is_required = true;
  constexpr static auto name = N;
};

namespace detail {

template <typename...>
struct parameter_executor {};

template <char const N[], typename T, bool required, typename default_v>
struct parameter_executor<factory_simple_parameter<N, T, required, default_v>> {
  using value_type = T;
  using result_type = result<std::pair<T, bool>, deserialize_error>;
  constexpr static bool has_value = true;

  static auto unpack(Slice s) {
    using namespace std::string_literals;

    auto value_slice = s.get(N);
    if (!value_slice.isNone()) {
      ensure_value_reader<T>{};

      return value_reader<T>::read(value_slice)
          .visit(visitor{[](T const& v) {
                           return result_type{std::make_pair(v, true)};
                         },
                         [](deserialize_error const& e) {
                           return result_type{
                               e.wrap("when reading value of field "s + N)};
                         }});
    }

    if (required) {
      return result_type{deserialize_error{"field "s + N + " is required"}};
    } else {
      return result_type{std::make_pair(default_v::value, false)};
    }
  }
};

template <char const N[], bool required>
struct parameter_executor<factory_slice_parameter<N, required>> {
  using parameter_type = factory_slice_parameter<N, required>;
  using value_type = typename parameter_type::value_type;
  using result_type = result<std::pair<value_type, bool>, deserialize_error>;
  constexpr static bool has_value = true;

  static auto unpack(Slice s) -> result_type {
    auto value_slice = s.get(N);
    if (!value_slice.isNone()) {
      return result_type{std::make_pair(value_slice, true)};
    }

    using namespace std::string_literals;

    if (required) {
      return result_type{deserialize_error{"field "s + N + " is required"}};
    } else {
      return result_type{std::make_pair(parameter_type::default_value, false)};
    }
  }
};

template <const char N[], typename V>
struct parameter_executor<expected_value<N, V>> {
  using parameter_type = expected_value<N, V>;
  using value_type = void;
  using result_type = result<std::pair<unit_type, bool>, deserialize_error>;
  constexpr static bool has_value = false;

  static auto unpack(Slice s) -> result_type {
    auto value_slice = s.get(N);
    values::ensure_value_comparator<V>{};
    if (values::value_comparator<V>::compare(value_slice)) {
      return result_type{std::make_pair(unit_type{}, true)};
    }

    using namespace std::string_literals;

    return result_type{deserialize_error{
        "value at `"s + N + "` not as expected, found: `" +
        value_slice.toJson() + "`, expected: `" + to_string(V{}) + "`"}};
  }
};

}  // namespace detail

}  // namespace parameter_list
namespace executor {

template <typename... T>
struct plan_result_tuple<parameter_list::parameter_list<T...>> {
  using type =
      typename ::deserializer::detail::gadgets::tuple_no_void<typename T::value_type...>::type;
};

namespace detail {
/*
 * I counts the number of visited fields.
 * K encodes the offset in the result tuple.
 */

template <int I, int K, typename...>
struct parameter_list_executor;

template <int I, int K, typename P, typename... Ps>
struct parameter_list_executor<I, K, parameter_list::parameter_list<P, Ps...>> {
  using unpack_result = result<unit_type, deserialize_error>;

  template <typename T>
  static auto unpack(T& t, Slice s) -> unpack_result {
    static_assert(::deserializer::detail::gadgets::is_complete_type_v<parameter_list::detail::parameter_executor<P>>,
                  "parameter executor is not defined");
    // maybe one can do this with folding?
    using executor = parameter_list::detail::parameter_executor<P>;
    using namespace std::string_literals;

    // expect_value does not have a value
    if constexpr (executor::has_value) {
      auto result = executor::unpack(s);
      if (result) {
        auto& [value, read_value] = result.get();
        std::get<I>(t) = value;
        if (read_value) {
          return parameter_list_executor<I + 1, K + 1, parameter_list::parameter_list<Ps...>>::unpack(t, s);
        } else {
          return parameter_list_executor<I + 1, K, parameter_list::parameter_list<Ps...>>::unpack(t, s);
        }
      }
      return unpack_result{result.error().wrap(
          "during read of "s + std::to_string(I) + "th parameters value")};
    } else {
      auto result = executor::unpack(s);
      if (result) {
        return parameter_list_executor<I, K + 1, parameter_list::parameter_list<Ps...>>::unpack(t, s);
      }
      return unpack_result{result.error()};
    }
  }
};

template <int I, int K>
struct parameter_list_executor<I, K, parameter_list::parameter_list<>> {
  using unpack_result = result<unit_type, deserialize_error>;

  template <typename T>
  static auto unpack(T& t, Slice s) -> unpack_result {
    if (s.length() != K) {
      return unpack_result{deserialize_error{
          "superfluous field in object, object has " + std::to_string(s.length()) +
          " fields, plan has " + std::to_string(K) + " fields"}};
    }

    return unpack_result{unit_type{}};
  }
};
}  // namespace detail

template <typename... Ps>
struct deserialize_plan_executor<parameter_list::parameter_list<Ps...>> {
  using tuple_type =
      typename plan_result_tuple<parameter_list::parameter_list<Ps...>>::type;
  using unpack_result = result<tuple_type, deserialize_error>;
  static auto unpack(arangodb::velocypack::Slice s) -> unpack_result {

    // store all read parameter in this tuple
    tuple_type parameter;

    if (!s.isObject()) {
      return unpack_result{
          deserialize_error(std::string{"value is not an object"})};
    }

    // forward to the parameter execution
    auto parameter_result =
        detail::parameter_list_executor<0, 0, parameter_list::parameter_list<Ps...>>::unpack(parameter, s);
    if (parameter_result) {
      return unpack_result{parameter};
    }

    return unpack_result{parameter_result.error()};
  }
};

}  // namespace executor
}  // namespace deserializer

#endif  // VELOCYPACK_PARAMETER_LIST_H

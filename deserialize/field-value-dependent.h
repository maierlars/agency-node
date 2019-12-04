#ifndef VELOCYPACK_FIELD_VALUE_DEPENDENT_H
#define VELOCYPACK_FIELD_VALUE_DEPENDENT_H
#include "plan-executor.h"
#include "values.h"

namespace deserializer {

namespace field_value_dependent {

/*
 * Selects a deserializer depending on the value of the field `N`. `VSs` is a
 * list of `value_deserializer_pairs` that are used in that process.
 */
template <const char N[], typename... VSs>
struct field_value_dependent;

template <typename V, typename S>
struct value_deserializer_pair {
  using value = V;
  using deserializer = S;
};

template <typename F, typename C = void>
class is_value_deserializer_pair : public std::true_type {};
template <typename F>
class is_value_deserializer_pair<F, std::void_t<typename F::value, typename F::deserializer>>
    : public std::true_type {
  static_assert(is_deserializer_v<typename F::deserializer>);
};

template <typename F>
constexpr bool is_value_deserializer_pair_v = is_value_deserializer_pair<F>::value;

template <const char N[], typename... VSs>
struct field_value_dependent {
  constexpr static auto name = N;

  using constructed_type = std::variant<typename VSs::deserializer::constructed_type...>;

  static_assert((is_value_deserializer_pair_v<VSs> && ...),
                "list shall only contain `value_deserializer_pair`s");
};

namespace detail {

template <typename...>
struct field_value_dependent_executor;
template <typename R, typename VD, typename... VDs>
struct field_value_dependent_executor<R, VD, VDs...> {
  using V = typename VD::value;
  using D = typename VD::deserializer;
  using unpack_result = result<R, deserialize_error>;
  static auto unpack(arangodb::velocypack::Slice s, arangodb::velocypack::Slice v)
      -> unpack_result {
    using namespace std::string_literals;
    values::ensure_value_comparator<V>{};
    if (values::value_comparator<V>::compare(v)) {
      return deserialize_with<D>(s).visit(
          visitor{[](auto const& v) { return unpack_result{R{v}}; },
                  [](deserialize_error const& e) {
                    return unpack_result{
                        e.wrap("during dependent parse with value `"s + to_string(V{}) + "`")};
                  }});
    }

    return field_value_dependent_executor<R, VDs...>::unpack(s, v);
  }
};

template <typename R>
struct field_value_dependent_executor<R> {
  using unpack_result = result<R, deserialize_error>;
  static auto unpack(arangodb::velocypack::Slice s, arangodb::velocypack::Slice v)
      -> unpack_result {
    using namespace std::string_literals;
    return unpack_result{deserialize_error{"no handler for value `"s + v.toJson() + "` known"}};
  }
};
}  // namespace detail

}  // namespace field_value_dependent

namespace executor {
template <const char N[], typename... VSs>
struct plan_result_tuple<field_value_dependent::field_value_dependent<N, VSs...>> {
  using variant = typename field_value_dependent::field_value_dependent<N, VSs...>::constructed_type;
  using type = std::tuple<variant>;
};

template <const char N[], typename... VSs>
struct deserialize_plan_executor<field_value_dependent::field_value_dependent<N, VSs...>> {
  using plan_result_tuple_type =
      plan_result_tuple<field_value_dependent::field_value_dependent<N, VSs...>>;
  using unpack_tuple_type = typename plan_result_tuple_type::type;
  using variant_type = typename plan_result_tuple_type::variant;
  using unpack_result = result<unpack_tuple_type, deserialize_error>;
  static auto unpack(arangodb::velocypack::Slice s) -> unpack_result {
    /*
     * Select the sub deserializer depending on the value.
     * Delegate to that deserializer.
     */
    using namespace std::string_literals;

    Slice value_slice = s.get(N);
    if (value_slice.isNone()) {
      return unpack_result{deserialize_error{"field `"s + N + "` not found"}};
    }

    return field_value_dependent::detail::field_value_dependent_executor<variant_type, VSs...>::unpack(s, value_slice)
        .visit(visitor{[](variant_type const& v) {
                         return unpack_result{std::make_tuple(v)};
                       },
                       [](deserialize_error const& e) {
                         return unpack_result{
                             e.wrap("when parsing dependently on `"s + N + "`")};
                       }});
  }
};

template <const char N[]>
struct deserialize_plan_executor<field_value_dependent::field_value_dependent<N>> {
  static auto unpack(arangodb::velocypack::Slice s) {
    /*
     * No matching type was found, we can not deserialize.
     */
    return result<unit_type, deserialize_error>{
        deserialize_error{"empty dependent field list"}};
  }
};

}  // namespace executor

}  // namespace deserializer

#endif  // VELOCYPACK_FIELD_VALUE_DEPENDENT_H

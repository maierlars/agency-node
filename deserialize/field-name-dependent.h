#ifndef VELOCYPACK_FIELD_NAME_DEPENDENT_H
#define VELOCYPACK_FIELD_NAME_DEPENDENT_H
#include "plan-executor.h"
#include "types.h"
#include "vpack-types.h"

namespace deserializer {

namespace field_name_dependent {

/*
 * field_name_dependent selects the deserializer by looking at the available
 * fields in the object. It takes the first deserializer that matches.
 */

template <const char N[], typename D>
struct field_name_deserializer_pair {
  constexpr static auto name = N;
  using deserializer = D;
};

// TODO add static_asserts like for field_value_depenent

template <typename... NDs>
struct field_name_dependent {
  using constructed_type = std::variant<typename NDs::deserializer::constructed_type...>;
};

namespace detail {
template <typename...>
struct field_name_dependent_executor;

template <typename R, const char N[], typename D, const char... Ns[], typename... Ds>
struct field_name_dependent_executor<R, field_name_deserializer_pair<N, D>, field_name_deserializer_pair<Ns, Ds>...> {
  using unpack_result = result<R, deserialize_error>;
  static auto unpack(::deserializer::slice_type s) -> unpack_result {
    using namespace std::string_literals;

    auto keySlice = s.get(N);

    if (!keySlice.isNone()) {
      return deserialize_with<D, hints::hint_list<hints::has_field<N>>>(s, std::make_tuple(keySlice))
          .visit(visitor{[](auto const& v) { return unpack_result{R{v}}; },
                         [](deserialize_error const& e) {
                           return unpack_result{
                               e.wrap("during dependent parse (found field `"s + N + "`)")};
                         }});
    }

    return field_name_dependent_executor<R, field_name_deserializer_pair<Ns, Ds>...>::unpack(s);
  }
};

template <typename R>
struct field_name_dependent_executor<R> {
  using unpack_result = result<R, deserialize_error>;
  static auto unpack(::deserializer::slice_type s) -> unpack_result {
    using namespace std::string_literals;
    return unpack_result{deserialize_error{"no known field"}};
  }
};

}  // namespace detail

}  // namespace field_name_dependent

namespace executor {

template <typename... NDs>
struct plan_result_tuple<field_name_dependent::field_name_dependent<NDs...>> {
  using variant = typename field_name_dependent::field_name_dependent<NDs...>::constructed_type;
  using type = std::tuple<variant>;
};

template <typename... NDs, typename H>
struct deserialize_plan_executor<field_name_dependent::field_name_dependent<NDs...>, H> {
  using value_type = typename field_name_dependent::field_name_dependent<NDs...>::constructed_type;
  using variant_type =
      typename plan_result_tuple<field_name_dependent::field_name_dependent<NDs...>>::variant;
  using tuple_type = std::tuple<value_type>;
  using result_type = result<tuple_type, deserialize_error>;

  static auto unpack(::deserializer::slice_type s, typename H::state_type hints) -> result_type {
    return field_name_dependent::detail::field_name_dependent_executor<variant_type, NDs...>::unpack(s)
        .visit(visitor{[](variant_type const& v) {
                         return result_type{std::make_tuple(v)};
                       },
                       [](deserialize_error const& e) { return result_type{e}; }});
  }
};
}  // namespace executor
}  // namespace deserializer

#endif  // VELOCYPACK_FIELD_NAME_DEPENDENT_H

#ifndef DESERIALIZER_ATTRIBUTE_H
#define DESERIALIZER_ATTRIBUTE_H
#include "errors.h"
#include "plan-executor.h"
#include "types.h"
#include "utilities.h"
#include "hints.h"
#include "deserialize-with.h"

namespace deserializer {

/*
 * Deserializes the value of the attribute `N` using the deserializer D.
 */
template <const char N[], typename D>
struct attribute_deserializer {
  constexpr static auto name = N;
  using plan = attribute_deserializer<N, D>;
  using constructed_type = typename D::constructed_type;
  using factory = utilities::identity_factory<constructed_type>;
};

namespace executor {

template <const char N[], typename D, typename H>
struct deserialize_plan_executor<attribute_deserializer<N, D>, H> {
  using tuple_type = std::tuple<typename attribute_deserializer<N, D>::constructed_type>;
  using result_type = result<tuple_type, deserialize_error>;

  auto static unpack(slice_type s, typename H::state_type hints) -> result_type {

    // if there is no hint that s is actually an object, we have to check that
    if constexpr (!hints::hint_is_object<H>) {
      if (!s.isObject()) {
        return result_type{deserialize_error{"object expected"}};
      }
    }

    slice_type value_slice;
    if constexpr (hints::hint_has_key<N, H>) {
      value_slice = hints::hint_list_state<hints::has_field<N>, H>::get(hints);
    } else {
      value_slice = s.get(N);
    }


    return deserialize_with<D>(value_slice).map([](auto v) { return std::make_tuple(v); });
  }
};

}  // namespace executor
}  // namespace deserializer

#endif  // DESERIALIZER_ATTRIBUTE_H

#ifndef VELOCYPACK_ARRAY_H
#define VELOCYPACK_ARRAY_H
#include "deserialize-with.h"
#include "errors.h"
#include "types.h"
#include "vpack-types.h"
#include "utilities.h"

namespace deserializer {

namespace arrays {

/*
 * Array deserializer is used to deserialize an array of variable length but with
 * homogeneous types. Each entry is deserialized with the given deserializer.
 */
template <typename D, template <typename> typename C>
using array_deserializer_constructed_type = C<typename D::constructed_type>;

template <typename D, template <typename> typename C,
          typename F = utilities::identity_factory<array_deserializer_constructed_type<D, C>>>
struct array_deserializer {
  using plan = array_deserializer<D, C, F>;
  using factory = F;
  using constructed_type = array_deserializer_constructed_type<D, C>;
};

}  // namespace arrays

namespace executor {

template <typename D, template <typename> typename C, typename F, typename H>
struct deserialize_plan_executor<arrays::array_deserializer<D, C, F>, H> {
  using container_type = typename arrays::array_deserializer<D, C, F>::constructed_type;
  using tuple_type = std::tuple<container_type>;
  using result_type = result<tuple_type, deserialize_error>;

  static auto unpack(slice_type slice, typename H::state_type) -> result_type {
    if (!slice.isArray()) {
      return result_type{deserialize_error{"array expected"}};
    }

    using namespace std::string_literals;
    std::size_t index = 0;
    container_type result;

    for (auto const& member : ::deserializer::array_iterator(slice)) {
      auto member_result = deserialize_with<D, hints::hint_list_empty>(member);
      if (member_result) {
        result.emplace_back(member_result.get());
      } else {
        return result_type{
            member_result.error().wrap("at position "s + std::to_string(index))};
      }
    }

    return result_type{std::move(result)};
  }
};

}  // namespace executor
}  // namespace deserializer

#endif  // VELOCYPACK_ARRAY_H
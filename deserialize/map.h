#ifndef VELOCYPACK_MAP_H
#define VELOCYPACK_MAP_H
#include "plan-executor.h"
#include "utilities.h"
#include "vpack-types.h"

namespace deserializer {

namespace map {

/*
 * Map deserializer deserializes an object by constructing a container that maps
 * keys (std::string_view) to constructed types from a second deserializer.
 */

// TODO introduce key_reader to support different types for keys (currently only std::string_view)

template <template <typename, typename> typename C, typename D, typename K>
using map_deserializer_constructed_type =
    C<typename K::value_type, typename D::constructed_type>;

using default_key_read = value_reader<std::string>;

template <typename D, template <typename, typename> typename C, typename K = default_key_read,
          typename F = utilities::identity_factory<map_deserializer_constructed_type<C, D, K>>>
struct map_deserializer {
  using plan = map_deserializer<D, C, K, F>;
  using factory = F;
  using constructed_type = map_deserializer_constructed_type<C, D, K>;
};

}  // namespace map

namespace executor {

template <typename D, template <typename, typename> typename C, typename F, typename K, typename H>
struct deserialize_plan_executor<map::map_deserializer<D, C, K, F>, H> {
  using container_type = typename map::map_deserializer<D, C, K, F>::constructed_type;
  using tuple_type = std::tuple<container_type>;
  using result_type = result<tuple_type, deserialize_error>;
  static auto unpack(::deserializer::slice_type s, typename H::state_type hints) -> result_type {
    container_type result;
    using namespace std::string_literals;

    if (!s.isObject()) {
      return result_type{deserialize_error{"expected object"}};
    }

    for (auto const& member : ::deserializer::object_iterator(s, true)) {  // use sequential deserialization
      auto member_result = deserialize_with<D>(member.value);
      if (!member_result) {
        return result_type{member_result.error().wrap(
            "when handling member `"s + member.key.copyString() + "`")};
      }

      auto key_result = K::read(member.key);
      if (!key_result) {
        return result_type{member_result.error().wrap("when reading key")};
      }

      result.insert(result.cend(),
                    std::make_pair(key_result.get(), member_result.get()));
    }

    return result_type{std::move(result)};
  }
};

}  // namespace executor

}  // namespace deserializer

#endif  // VELOCYPACK_MAP_H
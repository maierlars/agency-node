#ifndef VELOCYPACK_UTILITIES_H
#define VELOCYPACK_UTILITIES_H
#include <memory>
#include "gadgets.h"

namespace deserializer::utilities {

template <typename T>
struct identity_factory {
  using constructed_type = T;
  T operator()(T t) const { return std::move(t); }
};

template <typename P>
struct make_unique_factory {
  using constructed_type = std::unique_ptr<P>;

  template <typename... S>
  auto operator()(S&&... s) -> constructed_type {
    return std::make_unique<P>(std::forward<S>(s)...);
  }
};

template <typename T, typename P = void>
struct constructor_factory {
  using constructed_type = T;
  using plan = P;

  template <typename... S>
  T operator()(S&&... s) const {
    static_assert(detail::gadgets::is_braces_constructible_v<T, S...>,
                  "the type is not constructible with the given types");
    return T{std::forward<S>(s)...};
  }
};

template <typename T, typename P>
struct constructing_deserializer {
  using constructed_type = T;
  using plan = P;
  using factory = constructor_factory<T>;
};

}  // namespace deserializer::utilities

#endif  // VELOCYPACK_UTILITIES_H

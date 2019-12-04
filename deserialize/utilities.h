#ifndef VELOCYPACK_UTILITIES_H
#define VELOCYPACK_UTILITIES_H

namespace deserializer::utilities {

template <typename T>
struct identity_factory {
  using constructed_type = T;
  T operator()(T t) const { return std::move(t); }
};

template<typename T, typename P = void>
struct constructor_factory {
  using constructed_type = T;
  using plan = P;

  template<typename... S>
  T operator()(S&&... s) const {
    return T(std::forward<S>(s)...);
  }
};

}


#endif  // VELOCYPACK_UTILITIES_H

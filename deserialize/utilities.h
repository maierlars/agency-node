#ifndef VELOCYPACK_UTILITIES_H
#define VELOCYPACK_UTILITIES_H

namespace deserializer::utilities {

template <typename T>
struct identity_factory {
  using constructed_type = T;
  T operator()(T t) const { return std::move(t); }
};

}


#endif  // VELOCYPACK_UTILITIES_H

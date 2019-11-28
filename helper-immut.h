#ifndef AGENCY_HELPER_H
#define AGENCY_HELPER_H

#include <optional>

#include "velocypack/Buffer.h"
#include "velocypack/Builder.h"
#include "velocypack/Iterator.h"
#include "velocypack/Parser.h"
#include "velocypack/Slice.h"

using namespace arangodb::velocypack;

using VPackBufferPtr = std::shared_ptr<Buffer<uint8_t>>;

static inline Buffer<uint8_t> vpackFromJsonString(char const* c) {
  Options options;
  options.checkAttributeUniqueness = true;
  Parser parser(&options);
  parser.parse(c);

  std::shared_ptr<Builder> builder = parser.steal();
  std::shared_ptr<Buffer<uint8_t>> buffer = builder->steal();
  Buffer<uint8_t> b = std::move(*buffer);
  return b;
}

static inline Buffer<uint8_t> operator"" _vpack(const char* json, size_t) {
  return vpackFromJsonString(json);
}

template <class... Ts>
struct visitor : Ts... {
  using Ts::operator()...;
};
template <class... Ts>
visitor(Ts...)->visitor<Ts...>;

template <typename T>
struct immut_list {
  template <typename S>
  struct element {
    using pointer = std::shared_ptr<element<S>>;

    S value;
    std::shared_ptr<element<S>> next;

    explicit element(S v) : value(std::move(v)), next(nullptr) {}
  };

  using element_pointer = typename element<T>::pointer;
  element_pointer head;

  [[nodiscard]] bool empty() const noexcept { return head == nullptr; }

  immut_list(std::initializer_list<T> l) : head(nullptr) {
    typename element<T>::pointer last = nullptr;
    for (auto const& v : l) {
      auto next = std::make_shared<element<T>>(v);
      if (head == nullptr) {
        head = next;
      } else {
        last->next = next;
      }
      last = next;
    }
  }

  immut_list() : head(nullptr) {}
  explicit immut_list(typename element<T>::pointer head)
      : head(std::move(head)) {}
};

namespace std {
template <typename T>
struct tuple_size<immut_list<T>> {
  constexpr static auto value = 2;
};

template <typename T>
struct tuple_element<0, immut_list<T>> {
  using type = T;
};

template <typename T>
struct tuple_element<1, immut_list<T>> {
  using type = immut_list<T>;
};

template <std::size_t I, typename T>
auto get(immut_list<T> const& l) -> typename std::tuple_element<I, immut_list<T>>::type {
  static_assert(0 <= I && I <= 1);

  if constexpr (I == 0) {
    return l.head->value;
  } else if constexpr (I == 1) {
    return immut_list<T>{l.head->next};
  }
}
}  // namespace std

#endif

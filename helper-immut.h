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

template<typename T>
struct immutable_list {

  immutable_list() noexcept = default;
  immutable_list(immutable_list const&) = default;
  immutable_list(immutable_list&&) noexcept = default;

  immutable_list& operator=(immutable_list const&) = default;
  immutable_list& operator=(immutable_list&&) noexcept = default;

  immutable_list(std::initializer_list<T> list) noexcept {
    container_pointer current;
    for (auto &element : list) {
      container_pointer next = std::make_shared<element_container>(nullptr, element);
      if (_head == nullptr) {
        _head = next;
      } else {
        current->next = next;
      }
      current = next;
    }
  };

  immutable_list<T> push_front(T t) const noexcept {
    return immutable_list(std::make_shared<element_container>(_head, std::move(t)));
  }

  template<typename... As>
  immutable_list<T> emplace_front(As&&... a) const noexcept {
    return immutable_list(std::make_shared<element_container>(_head, std::forward<As>(a)...));
  }

  explicit operator bool() const noexcept { return _head.operator bool(); }
  [[nodiscard]] bool empty() const noexcept { return _head; }
  immutable_list<T> tail() const noexcept { return immutable_list(_head ? _head->next : nullptr); }
  T const& head() const noexcept { return *_head; }

  bool operator==(immutable_list<T> const& other) const noexcept {
    if (_head == nullptr || other._head == nullptr) {
      return _head == other._head;
    }

    if (_head->value == other._head->value) {
      return tail() == other.tail();
    }

    return false;
  }

  bool has_prefix(immutable_list<T> const& prefix) const noexcept {
    if (prefix._head == nullptr) {
      return true;
    }
    if (_head == nullptr) {
      return false;
    }

    if (_head->value == prefix._head->value) {
      return tail().has_prefix(prefix.tail());
    }

    return false;
  }

  bool operator<(immutable_list<T> const& other) const noexcept {
    if (other._head == nullptr) {
      return false;
    }
    if (_head == nullptr) {
      return true;
    }

    if (_head->value < other._head->value) {
      return tail() < other.tail();
    }
    return false;
  }
 private:
  struct element_container;
  using container_pointer = std::shared_ptr<element_container>;

  struct element_container {
    container_pointer next;
    T value;

    template<typename... As>
    explicit element_container(container_pointer next, As&&... a) : next(std::move(next)), value(std::forward<As>(a)...) {}
    explicit element_container(container_pointer next, T value) : next(std::move(next)), value(std::move(value)) {}
  };


  explicit immutable_list(container_pointer head) noexcept : _head(std::move(head)) {}
  container_pointer _head;
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

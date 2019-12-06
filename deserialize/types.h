#ifndef VELOCYPACK_TYPES_H
#define VELOCYPACK_TYPES_H
#include <variant>

struct unit_type {};

template <typename T, typename E>
struct result {
  std::variant<T, E> value;

  static_assert(!std::is_void_v<T> && !std::is_void_v<E>);

  template <typename S>
  explicit result(result<S, E> const& r) {
    if (r) {
      value.emplace(r.get());
    } else {
      value.emplace(r.error());
    }
  }

  template <typename S>
  explicit result(S&& s) : value(std::forward<S>(s)) {}

  explicit operator bool() const noexcept {
    return std::holds_alternative<T>(value);
  }

  template <typename F>
  auto visit(F&& f) const {
    return std::visit(f, value);
  }

  E& error() { return std::get<E>(value); }
  [[nodiscard]] E const& error() const { return std::get<E>(value); }

  T& get() { return std::get<T>(value); }
  [[nodiscard]] T const& get() const { return std::get<T>(value); }

  template <typename F, typename R = std::invoke_result_t<F, T const&>>
  result<R, E> map(F&& f) const {
    if (*this) {
      return result<R, E>(f(get()));
    } else {
      return result<R, E>(error());
    }
  }

  [[nodiscard]] result<T, E> wrap_error(std::string const& msg) const {
    if (*this) {
      return *this;
    }

    return result<T, E>(error().wrap(msg));
  }
};
#endif  // VELOCYPACK_TYPES_H

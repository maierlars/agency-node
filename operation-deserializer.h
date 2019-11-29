//
// Created by lars on 28.11.19.
//

#ifndef AGENCY_OPERATION_DESERIALIZER_H
#define AGENCY_OPERATION_DESERIALIZER_H
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <variant>
#include <vector>

#include "velocypack/Buffer.h"
#include "velocypack/Builder.h"
#include "velocypack/Iterator.h"
#include "velocypack/Parser.h"
#include "velocypack/Slice.h"

template <char const N[], typename T, bool required>
struct operation_parameter {
  using value_type = T;
  constexpr static bool is_required = required;
  constexpr static auto name = N;
};

template <char const N[], bool required>
struct operation_slice_parameter {
  using value_type = arangodb::velocypack::Slice;
  constexpr static bool is_required = required;
  constexpr static auto name = N;
};

constexpr const char parameter_name_delta[] = "delta";
constexpr const char parameter_name_new[] = "new";

using operation_delta_parameter = operation_parameter<parameter_name_delta, double, false>;
using operation_new_parameter = operation_parameter<parameter_name_new, double, true>;

template <typename... T>
using operation_parameter_list = std::tuple<T...>;

using increment_parameter_list = operation_parameter_list<operation_delta_parameter>;

using set_parameter_list =
    operation_parameter_list<operation_new_parameter>;

using operation = double;

namespace detail {

template <typename>
struct parameter_reader;

template <>
struct parameter_reader<double> {
  std::optional<double> operator()(arangodb::velocypack::Slice s) const noexcept {
    if (s.isDouble()) {
      return s.getNumber<double>();
    }
    return std::nullopt;
  }
};

template <typename>
struct parameter_getter;

template <char const N[], typename T, bool req>
struct parameter_getter<operation_parameter<N, T, req>> {
  constexpr static bool required = req;

  std::optional<T> operator()(arangodb::velocypack::Slice s) const noexcept {
    if (s.hasKey(N)) {
      return parameter_reader<T>{}(s.get(N));
    }
    return std::nullopt;
  }
};

template <typename>
struct operation_applier;

template <char const N[], typename T, bool required>
struct operation_applier<operation_parameter<N, T, required>> {
  using op_type = operation_parameter<N, T, required>;
  constexpr static bool is_required = required;

  [[nodiscard]] std::optional<T> apply(arangodb::velocypack::Slice s) const noexcept {
    return parameter_getter<op_type>{}(s);
  }
};

template <typename, typename, typename>
struct operation_deserializer_exec;

template <typename F, typename T, typename... Ts, typename... V>
struct operation_deserializer_exec<F, operation_parameter_list<T, Ts...>, std::tuple<V...>> {
  explicit operation_deserializer_exec(F const& f) noexcept : f(f) {}

  [[nodiscard]] auto expand(arangodb::velocypack::Slice s, std::tuple<V...> v) const noexcept {
    /*
     *  Expand the current operation
     */

    auto const& value = operation_applier<T>{}.apply(s);

    if (value) {
      /*
       *  Forward to the next layer.
       */

      auto result =
          operation_deserializer_exec<F, operation_parameter_list<Ts...>,
                                      std::tuple<V..., decltype(value.value())>>{f}
              .expand(s, std::tuple_cat(v, std::make_tuple(value.value())));

      /*
       * If the current operation could not be expanded, exec.
       */
      if (result) {
        return result;
      }
    }

    if constexpr (operation_applier<T>::is_required) {
      // abort if the expansion was required
      return std::nullopt;
    } else {
      return exec(s, v);
    }
  }

  [[nodiscard]] auto exec(arangodb::velocypack::Slice s, std::tuple<V...> v) const noexcept
      -> std::optional<decltype(std::apply(std::declval<F>(), v))> {
    if (s.length() != sizeof...(V)) {
      return std::nullopt;
    }

    return std::apply(f, v);
  }

 private:
  F const& f;
};

template <typename F, typename... V>
struct operation_deserializer_exec<F, operation_parameter_list<>, std::tuple<V...>> {
  explicit operation_deserializer_exec(F const& f) noexcept : f(f) {}

  [[nodiscard]] auto expand(arangodb::velocypack::Slice s, std::tuple<V...> v) const noexcept {
    return exec(s, v);
  }

  [[nodiscard]] auto exec(arangodb::velocypack::Slice s, std::tuple<V...> v) const noexcept
      -> std::optional<decltype(std::apply(std::declval<F>(), v))> {
    if (s.length() != sizeof...(V)) {
      return std::nullopt;
    }

    return {std::apply(f, v)};
  }

 private:
  F const& f;
};

}  // namespace detail

template <typename P, typename F>
struct operation_deserializer {
  operation_deserializer(P&&, F&& f) : f(std::forward<F>(f)) {}

  auto operator()(arangodb::velocypack::Slice s) const noexcept {
    return detail::operation_deserializer_exec<F, P, std::tuple<>>{f}.expand(s, std::make_tuple<>());
  }

 private:
  F f;
};

static inline auto deserialze(arangodb::velocypack::Slice s) {
  return operation_deserializer{increment_parameter_list{}, [](double delta = 1.0) {
                                  return [delta](double x) { return x + delta; };
                                }}(s);
}

#endif  // AGENCY_OPERATION_DESERIALIZER_H

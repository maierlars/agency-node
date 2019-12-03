#ifndef VELOCYPACK_DESERIALIZER_H
#define VELOCYPACK_DESERIALIZER_H
#include <variant>

#include "velocypack/Slice.h"

struct deserialize_error {
  explicit deserialize_error(std::string msg) : msg(std::move(msg)) {}

  std::string const& what() const { return msg; }
  deserialize_error wrap(std::string const& wrap) const {
    return deserialize_error(wrap + ": " + msg);
  }

 private:
  std::string msg;
};

struct unit_type {};

template <typename T, typename E>
struct result {
  std::variant<T, E> value;

  template <typename S>
  result(result<S, E> const& r) {
    if (r) {
      value.emplace(r.get());
    } else {
      value.emplace(r.error());
    }
  }

  template <typename S>
  explicit result(S&& s) : value(std::forward<S>(s)) {}

  operator bool() const noexcept { return std::holds_alternative<T>(value); }

  template <typename F>
  auto visit(F&& f) const {
    return std::visit(f, value);
  }

  E& error() { return std::get<E>(value); }
  E const& error() const { return std::get<E>(value); }

  T& get() { return std::get<T>(value); }
  T const& get() const { return std::get<T>(value); }

  template <typename F, typename R = std::invoke_result_t<F, T const&>>
  result<R, E> map(F&& f) const {
    if (*this) {
      return result<R, E>(f(get()));
    } else {
      return result<R, E>(error());
    }
  }

  result<T, E> wrap_error(std::string const& msg) const {
    if (*this) {
      return *this;
    }

    return result<T, E>(error().wrap(msg));
  }
};

template <typename T>
struct value_type {
  using type = T;
};

template <typename T>
struct default_constructed_value : value_type<T> {
  constexpr static auto value = T{};
};

template <typename T, int V>
struct numeric_value : value_type<T> {
  constexpr static auto value = T{V};
};

template <const char V[]>
struct string_value : value_type<const char[]> {
  constexpr static auto value = V;
};

template <typename T, int V>
std::string to_string(numeric_value<T, V> const&) {
  return std::to_string(V);
}

template <const char V[]>
std::string to_string(string_value<V> const&) {
  return V;
}

template <char const N[], typename T, bool required, typename default_v = default_constructed_value<T>>
struct factory_simple_parameter {
  using value_type = T;
  constexpr static bool is_required = required;
  constexpr static auto name = N;
  constexpr static auto default_value = default_v::value;
};

template <char const N[], bool required>
struct factory_slice_parameter {
  using value_type = arangodb::velocypack::Slice;
  constexpr static bool is_required = required;
  constexpr static auto name = N;
  constexpr static auto default_value = arangodb::velocypack::Slice::nullSlice();
};

template <const char N[], typename V>
struct expected_value {
  using value_type = void;
  using value = V;
  constexpr static bool is_required = true;
  constexpr static auto name = N;
};

template <typename V>
struct value_slice_comparator;

template <typename T, int v>
struct value_slice_comparator<numeric_value<T, v>> {
  static bool compare(Slice s) {
    if (s.isNumber<T>()) {
      return s.getNumericValue<T>() == numeric_value<T, v>::value;
    }

    return false;
  }
};

template <const char V[]>
struct value_slice_comparator<string_value<V>> {
  static bool compare(Slice s) {
    if (s.isString()) {
      return s.isEqualString(arangodb::velocypack::StringRef(V, strlen(V)));
    }

    return false;
  }
};

template <typename T>
struct value_reader;

template <>
struct value_reader<double> {
  using result_type = result<double, deserialize_error>;
  static result_type read(Slice s) {
    if (s.isNumber()) {
      return result_type{s.getNumber<double>()};
    }

    return result_type{deserialize_error{"value is not a double"}};
  }
};

template <typename V, typename S>
struct value_deserializer_pair {
  using value = V;
  using deserializer = S;
};

template <const char N[], typename... VSs>
struct field_dependent {
  constexpr static auto name = N;

  using constructed_type = std::variant<typename VSs::deserializer::constructed_type...>;
};

template <typename... T>
struct parameter_list {
  constexpr static auto length = sizeof...(T);
};

template<typename D, template<typename, typename> typename C>
struct map_deserializer {
  using constructed_type = C<std::string_view, typename D::constructed_type>;
};


namespace detail {

template <typename... Ts>
struct tuple_no_void_impl;

template <typename... Ts, typename E, typename... Es>
struct tuple_no_void_impl<std::tuple<Ts...>, E, Es...> {
  using type = typename tuple_no_void_impl<std::tuple<Ts..., E>, Es...>::type;
};

template <typename... Ts, typename... Es>
struct tuple_no_void_impl<std::tuple<Ts...>, void, Es...> {
  using type = typename tuple_no_void_impl<std::tuple<Ts...>, Es...>::type;
};

template <typename... Ts>
struct tuple_no_void_impl<std::tuple<Ts...>> {
  using type = std::tuple<Ts...>;
};
}  // namespace detail

template <typename... Ts>
struct tuple_no_void {
  using type = typename detail::tuple_no_void_impl<std::tuple<>, Ts...>::type;
};

template <typename T>
struct plan_result_tuple {
  using type = std::tuple<typename T::constructed_type>;
};

template <typename... T>
struct plan_result_tuple<parameter_list<T...>> {
  using type = typename tuple_no_void<typename T::value_type...>::type;
};

template <const char N[], typename... VSs>
struct plan_result_tuple<field_dependent<N, VSs...>> {
  using variant = typename field_dependent<N, VSs...>::constructed_type;
  using type = std::tuple<variant>;
};

template <typename P, typename F, typename R = typename F::constructed_type>
struct deserializer {
  using plan = P;
  using factory = F;
  using constructed_type = R;
};

template <typename F>
struct deserializer_from_factory
    : deserializer<typename F::plan, F, typename F::constructed_type> {};

template <typename...>
struct parameter_executor {};

template <char const N[], typename T, bool required, typename default_v>
struct parameter_executor<factory_simple_parameter<N, T, required, default_v>> {
  using value_type = T;
  using result_type = result<std::pair<T, bool>, deserialize_error>;
  constexpr static bool has_value = true;

  static auto unpack(Slice s) {
    using namespace std::string_literals;

    auto value_slice = s.get(N);
    if (!value_slice.isNone()) {
      return value_reader<T>::read(value_slice).visit(visitor{
        [](T const& v) { return result_type{std::make_pair(v, true)}; },
        [](deserialize_error const& e) { return result_type{e.wrap("when reading value of field "s + N)}; }
      });
    }

    if (required) {
      return result_type{deserialize_error{"field "s + N + " is required"}};
    } else {
      return result_type{std::make_pair(default_v::value, false)};
    }
  }
};

template <char const N[], bool required>
struct parameter_executor<factory_slice_parameter<N, required>> {
  using parameter_type = factory_slice_parameter<N, required>;
  using value_type = typename parameter_type::value_type;
  using result_type = result<std::pair<value_type, bool>, deserialize_error>;
  constexpr static bool has_value = true;

  static auto unpack(Slice s) -> result_type {
    auto value_slice = s.get(N);
    if (!value_slice.isNone()) {
      return result_type{std::make_pair(value_slice, true)};
    }

    using namespace std::string_literals;

    if (required) {
      return result_type{deserialize_error{"field "s + N + " is required"}};
    } else {
      return result_type{std::make_pair(parameter_type::default_value, false)};
    }
  }
};

template <const char N[], typename V>
struct parameter_executor<expected_value<N, V>> {
  using parameter_type = expected_value<N, V>;
  using value_type = void;
  using result_type = result<std::pair<unit_type, bool>, deserialize_error>;
  constexpr static bool has_value = false;

  static auto unpack(Slice s) -> result_type {
    auto value_slice = s.get(N);

    if (value_slice_comparator<V>::compare(value_slice)) {
      return result_type{std::make_pair(unit_type{}, true)};
    }

    using namespace std::string_literals;

    return result_type{deserialize_error{
        "value at `"s + N + "` not as expected, found: `" +
        value_slice.toJson() + "`, expected: `" + to_string(V{}) + "`"}};
  }
};

template <int I, int K, typename...>
struct parameter_list_executor;

template <int I, int K, typename P, typename... Ps>
struct parameter_list_executor<I, K, parameter_list<P, Ps...>> {
  using unpack_result = result<unit_type, deserialize_error>;

  template <typename T>
  static auto unpack(T& t, Slice s) -> unpack_result {
    // maybe one can do this with folding?
    using executor = parameter_executor<P>;

    using namespace std::string_literals;

    if constexpr (executor::has_value) {
      auto result = executor::unpack(s);
      if (result) {
        auto &[value, read_value] = result.get();
        std::get<I>(t) = value;
        if (read_value) {
          return parameter_list_executor<I + 1, K + 1, parameter_list<Ps...>>::unpack(t, s);
        } else{
          return parameter_list_executor<I + 1, K, parameter_list<Ps...>>::unpack(t, s);
        }
      }
      return unpack_result{result.error().wrap(
          "during read of "s + std::to_string(I) + "th parameters value")};
    } else {
      auto result = executor::unpack(s);
      if (result) {
        return parameter_list_executor<I, K + 1, parameter_list<Ps...>>::unpack(t, s);
      }
      return unpack_result{result.error()};
    }
  }
};

template <int I, int K>
struct parameter_list_executor<I, K, parameter_list<>> {
  using unpack_result = result<unit_type, deserialize_error>;

  template <typename T>
  static auto unpack(T& t, Slice s) -> unpack_result {
    if (s.length() != K) {
      return unpack_result{deserialize_error{"superfluous field in object, object has " + std::to_string(s.length()) + " fields, plan has " + std::to_string(K) + " fields"}};
    }

    return unpack_result{unit_type{}};
  }
};

template <typename P>
struct deserialize_plan_executor;

template <typename... Ps>
struct deserialize_plan_executor<parameter_list<Ps...>> {
  using tuple_type = typename plan_result_tuple<parameter_list<Ps...>>::type;
  using unpack_result = result<tuple_type, deserialize_error>;
  static auto unpack(arangodb::velocypack::Slice s) -> unpack_result {
    tuple_type parameter;

    if (!s.isObject()) {
      return unpack_result{
          deserialize_error(std::string{"value is not an object"})};
    }

    // forward to the parameter execution
    auto parameter_result =
        parameter_list_executor<0, 0, parameter_list<Ps...>>::unpack(parameter, s);
    if (parameter_result) {
      return unpack_result{parameter};
    }

    return unpack_result{parameter_result.error()};
  }
};

template <typename D>
auto deserialize_with(arangodb::velocypack::Slice slice)
    -> result<typename D::constructed_type, deserialize_error>;

template <typename...>
struct field_dependent_executor;

template <typename R, typename V, typename D, typename... Vs, typename... Ds>
struct field_dependent_executor<R, value_deserializer_pair<V, D>, value_deserializer_pair<Vs, Ds>...> {
  using unpack_result = result<R, deserialize_error>;
  static auto unpack(arangodb::velocypack::Slice s, arangodb::velocypack::Slice v)
      -> unpack_result {
    using namespace std::string_literals;

    if (value_slice_comparator<V>::compare(v)) {
      return deserialize_with<D>(s).visit(
          visitor{[](auto const& v) { return unpack_result{R{v}}; },
                  [](deserialize_error const& e) {
                    return unpack_result{e.wrap("during dependent parse with value `"s + to_string(V{}) + "`")};
                  }});
    }

    return field_dependent_executor<R, value_deserializer_pair<Vs, Ds>...>::unpack(s, v);
  }
};

template <typename R>
struct field_dependent_executor<R> {
  using unpack_result = result<R, deserialize_error>;
  static auto unpack(arangodb::velocypack::Slice s, arangodb::velocypack::Slice v)
      -> unpack_result {
    using namespace std::string_literals;
    return unpack_result{deserialize_error{"no handler for value `"s + v.toJson() + "` known"}};
  }
};

template <const char N[], typename... VSs>
struct deserialize_plan_executor<field_dependent<N, VSs...>> {
  using unpack_tuple_type = typename plan_result_tuple<field_dependent<N, VSs...>>::type;
  using variant_type = typename plan_result_tuple<field_dependent<N, VSs...>>::variant;
  using unpack_result = result<unpack_tuple_type, deserialize_error>;
  static auto unpack(arangodb::velocypack::Slice s) -> unpack_result {
    /*
     * Select the sub deserializer depending on the value.
     * Delegate to that deserializer.
     */
    using namespace std::string_literals;

    Slice value_slice = s.get(N);
    if (value_slice.isNone()) {
      return unpack_result{deserialize_error{"field `"s + N + "` not found"}};
    }

    return field_dependent_executor<variant_type, VSs...>::unpack(s, value_slice)
        .visit(visitor{[](variant_type const& v) {
                         return unpack_result{std::make_tuple(v)};
                       },
                       [](deserialize_error const& e) {
                         return unpack_result{
                             e.wrap("when parsing dependently on `"s + N + "`")};
                       }});
  }
};

template <const char N[]>
struct deserialize_plan_executor<field_dependent<N>> {
  static auto unpack(arangodb::velocypack::Slice s) {
    /*
     * No matching type was found, we can not deserialize.
     */
    return result<unit_type, deserialize_error>{
        deserialize_error{"empty dependent field list"}};
  }
};

template<typename D, template<typename, typename> typename C>
struct deserialize_plan_executor<map_deserializer<D, C>> {
  using container_type = typename map_deserializer<D, C>::constructed_type;
  using tuple_type = std::tuple<container_type>;
  using result_type = result<tuple_type, deserialize_error>;
  static auto unpack(arangodb::velocypack::Slice s) -> result_type {
    container_type result;
    using namespace std::string_literals;

    for (auto const& member : arangodb::velocypack::ObjectIterator(s)) {
      auto member_result = deserialize_with<D>(member.value);
      if (!member_result) {
        return result_type{member_result.error().wrap("when handling member `"s + member.key.copyString() + "`")};
      }

      result.insert(result.cend(), std::make_pair(member.key.stringView(), member_result.get()));
    }

    return result_type{std::move(result)};
  }

};

template <typename R, typename F, typename T>
struct is_applicable_r;

template <typename R, typename F, typename... Ts>
struct is_applicable_r<R, F, std::tuple<Ts...>> {
  constexpr static bool value = std::is_invocable_r_v<R, F, Ts...>;
};

template <typename D>
auto deserialize_with(arangodb::velocypack::Slice slice)
    -> result<typename D::constructed_type, deserialize_error> {
  using plan = typename D::plan;
  using factory = typename D::factory;
  using result_type = result<typename D::constructed_type, deserialize_error>;

  static_assert(is_applicable_r<typename D::constructed_type, factory, typename plan_result_tuple<plan>::type>::value,
                "factory is not callable with result of plan unpacking");

  auto plan_result = deserialize_plan_executor<plan>::unpack(slice);
  if (plan_result) {
    return result_type{std::apply(factory{}, plan_result.get())};
  }

  return result_type{plan_result.error()};
}

#endif  // VELOCYPACK_DESERIALIZER_H

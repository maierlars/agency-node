#ifndef VELOCYPACK_DESERIALIZE_WITH_H
#define VELOCYPACK_DESERIALIZE_WITH_H
#include "gadgets.h"
#include "types.h"
#include "plan-executor.h"
#include "vpack-types.h"

namespace deserializer {

template <typename F, typename C = void>
class is_factory : public std::false_type {};
template <typename F>
class is_factory<F, std::void_t<typename F::constructed_type>> : public std::true_type {
};
template <typename F>
constexpr bool is_factory_v = is_factory<F>::value;

template <typename F>
struct from_factory {
  using plan = typename F::plan;
  using factory = F;
  using constructed_type = typename F::constructed_type;

  static_assert(is_factory_v<F>, "factory does not have required fields");
};

/*
 * This is the prototype of every deserializer. `constructed_type` represents
 * the type that is constructed during deserialization. `plan` describes how the
 * object is read and `factory` is a callable object that is used to create the
 * result object with the values read during unpack phase.
 */
template <typename P, typename F, typename R = typename F::constructed_type>
struct deserializer {
  using plan = P;
  using factory = F;
  using constructed_type = R;
};

template <typename T, typename C = void>
class is_deserializer : public std::false_type {};
template <typename T>
class is_deserializer<T, std::void_t<typename T::plan, typename T::factory, typename T::constructed_type>>
    : public std::true_type {};
template <typename D>
constexpr bool is_deserializer_v = is_deserializer<D>::value;

template <typename D, typename F>
auto deserialize_with(F& factory, ::deserializer::slice_type slice) {
  static_assert(is_deserializer_v<D>,
                "given deserializer is missing some fields");

  using plan = typename D::plan;
  using factory_type = typename D::factory;
  using result_type = result<typename D::constructed_type, deserialize_error>;

  using plan_result_type = typename executor::plan_result_tuple<plan>::type;

  static_assert(detail::gadgets::is_applicable_r<typename D::constructed_type, factory_type, plan_result_type>::value,
                "factory is not callable with result of plan unpacking");

  static_assert(detail::gadgets::is_complete_type_v<executor::deserialize_plan_executor<plan>>,
                "plan type does not specialize deserialize_plan_executor. You "
                "will get an incomplete type error.");

  static_assert(std::is_invocable_r_v<result<plan_result_type, deserialize_error>,
                                      decltype(&executor::deserialize_plan_executor<plan>::unpack),
                                      ::deserializer::slice_type>,
                "executor::unpack does not have the correct signature");

  // Simply forward to the plan_executor.
  auto plan_result = executor::deserialize_plan_executor<plan>::unpack(slice);
  if (plan_result) {
    // if successfully deserialized, apply to the factory.
    return result_type(std::apply(factory, plan_result.get()));
  }
  // otherwise forward the error
  return result_type(plan_result.error());
}

/*
 * Deserializes the given slice using the deserializer D.
 */
template <typename D>
auto deserialize_with(::deserializer::slice_type slice) {
  using factory_type = typename D::factory;
  factory_type factory{};
  return deserialize_with<D>(factory, slice);
}
}  // namespace deserializer
#endif  // VELOCYPACK_DESERIALIZE_WITH_H

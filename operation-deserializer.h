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

#include "node-operations.h"
#include "velocypack/Buffer.h"
#include "velocypack/Builder.h"
#include "velocypack/Iterator.h"
#include "velocypack/Parser.h"
#include "velocypack/Slice.h"

#include "deserializer.h"
#include "node-operations.h"

constexpr const char parameter_name_delta[] = "delta";
constexpr const char parameter_name_new[] = "new";
constexpr const char parameter_name_op[] = "op";
constexpr const char parameter_name_ttl[] = "ttl";

constexpr const char parameter_op_name_set[] = "set";
constexpr const char parameter_op_name_increment[] = "increment";
constexpr const char parameter_op_name_decrement[] = "decrement";
constexpr const char parameter_op_name_remove[] = "remove";

using operation_delta_parameter =
    factory_simple_parameter<parameter_name_delta, double, false>;
using operation_ttl_parameter =
    factory_simple_parameter<parameter_name_ttl, double, false>;
using operation_new_parameter = factory_slice_parameter<parameter_name_new, true>;

using increment_parameter_list = parameter_list<operation_delta_parameter>;

using operation_op_set =
    expected_value<parameter_name_op, string_value<parameter_op_name_set>>;

struct increment_operation_factory {
  using plan = increment_parameter_list;
  using constructed_type = increment_operator;

  template <typename... T>
  constructed_type operator()(double delta) const {
    return increment_operator{delta};
  }
};

using agency_operation_plan =
    field_dependent<parameter_name_op, value_deserializer_pair<string_value<parameter_op_name_set>, deserializer_from_factory<increment_operation_factory>>>;

struct agency_operation_factory {
  using plan = agency_operation_plan;
  using constructed_type = std::function<node_ptr(node_ptr const&)>;

  template <typename... T>
  constructed_type operator()(std::variant<T...> const& v) const {
    return std::visit(
        [](auto const& v) -> constructed_type { return std::function{v}; }, v);
  }
};

using agency_operation_deserialzer = deserializer_from_factory<agency_operation_factory>;

#endif  // AGENCY_OPERATION_DESERIALIZER_H

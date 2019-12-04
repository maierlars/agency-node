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

#include "deserialize/deserializer.h"
#include "node-operations.h"

using namespace deserializer::field_value_dependent;
using namespace deserializer::field_name_dependent;
using namespace deserializer::fixed_order;
using namespace deserializer::map;
using namespace deserializer::parameter_list;
using namespace deserializer::values;

constexpr const char parameter_name_delta[] = "delta";
constexpr const char parameter_name_new[] = "new";
constexpr const char parameter_name_op[] = "op";
constexpr const char parameter_name_ttl[] = "ttl";

constexpr const char parameter_op_name_set[] = "set";
constexpr const char parameter_op_name_increment[] = "increment";
constexpr const char parameter_op_name_decrement[] = "decrement";
constexpr const char parameter_op_name_remove[] = "remove";

/* clang-format off */

using operation_delta_parameter = factory_simple_parameter<parameter_name_delta, double, false>;
using operation_ttl_parameter = factory_simple_parameter<parameter_name_ttl, double, false>;
using operation_new_parameter = factory_slice_parameter<parameter_name_new, true>;

using operation_op_set = expected_value<parameter_name_op, string_value<parameter_op_name_set>>;
using operation_op_increment = expected_value<parameter_name_op, string_value<parameter_op_name_increment>>;

struct increment_operation_factory {
  using plan = parameter_list<
      operation_op_increment,
      operation_delta_parameter>;

  using constructed_type = increment_operator;

  constructed_type operator()(double delta) const {
    return increment_operator{delta};
  }
};

struct set_operation_factory {
  using plan = parameter_list<
      operation_op_set,
      operation_new_parameter,
      operation_ttl_parameter>;
  using constructed_type = set_operator;

  constructed_type operator()(arangodb::velocypack::Slice value, double ttl) const {
    return set_operator(node::from_slice(value));
  }
};

struct agency_operation_factory {
  using plan = field_value_dependent<
      parameter_name_op,
      value_deserializer_pair<
          string_value<parameter_op_name_increment>,
          deserializer::from_factory<increment_operation_factory>>,
      value_deserializer_pair<
          string_value<parameter_op_name_set>,
          deserializer::from_factory<set_operation_factory>>>;
  using constructed_type = std::function<node_ptr(node_ptr const&)>;

  template <typename... T>
  constructed_type operator()(std::variant<T...> const& v) const {
    return std::visit(
        [](auto const& v) -> constructed_type { return std::function{v}; }, v);
  }
};
using agency_operation_deserialzer = deserializer::from_factory<agency_operation_factory>;


constexpr const char parameter_name_old[] = "old";
constexpr const char parameter_name_old_not[] = "oldNot";
constexpr const char parameter_name_old_empty[] = "oldEmpty";


using precondition_old_parameter = factory_slice_parameter<parameter_name_old, true>;
using precondition_old_not_parameter = factory_slice_parameter<parameter_name_old_not, true>;
using precondition_old_empty_parameter = factory_simple_parameter<parameter_name_old_empty, bool, true>;

template<typename C, typename P>
struct slice_condition_factory {
  using plan = P;
  using constructed_type = C;

  constructed_type operator()(arangodb::velocypack::Slice s) const {
    return constructed_type{node::from_slice(s)};
  }
};

using equal_condition_factory = slice_condition_factory<equal_condition, parameter_list<precondition_old_parameter>>;
using not_equal_condition_factory = slice_condition_factory<not_equal_condition, parameter_list<precondition_old_not_parameter>>;

struct old_empty_condition_factory {
  using plan = parameter_list<precondition_old_empty_parameter>;
  using constructed_type = is_empty_condition;

  constructed_type operator()(bool inverted) const {
    return constructed_type{inverted};
  }
};

struct agency_precondition_factory {
  using plan = field_name_dependent<
          field_name_deserializer_pair<parameter_name_old, deserializer::from_factory<equal_condition_factory>>,
          field_name_deserializer_pair<parameter_name_old_not, deserializer::from_factory<not_equal_condition_factory>>,
          field_name_deserializer_pair<parameter_name_old_empty, deserializer::from_factory<old_empty_condition_factory>>
      >;

  using constructed_type = std::function<bool(node_ptr const&)>;

  template <typename... T>
  constructed_type operator()(std::variant<T...> const& v) const {
    return std::visit(
        [](auto const& v) -> constructed_type { return std::function{v}; }, v);
  }
};

using agency_precondition_deserialzer = deserializer::from_factory<agency_precondition_factory>;


template <typename K, typename V>
using vector_map = std::vector<std::pair<K, V>>;

using operation_list = vector_map<std::string_view, node::modify_operation>;
using precondition_list = vector_map<std::string_view, node::fold_operator<bool>>;

struct agency_transaction {
  operation_list operations;
  precondition_list preconditions;
  std::string client_id;
};


struct agency_transaction_factory {
  using operation_deserializer = map_deserializer<agency_operation_deserialzer, vector_map>;
  using precondition_deserializer = map_deserializer<agency_precondition_deserialzer, vector_map>;

  using plan = fixed_order_deserializer<
      operation_deserializer, precondition_deserializer, value_deserializer<std::string>>;
  using constructed_type = agency_transaction;

  constructed_type operator()(const std::tuple<operation_deserializer::constructed_type, precondition_deserializer::constructed_type, std::string>& array) const {
    auto &[op_list, prec_list, client_id] = array;
    return constructed_type{op_list, prec_list, client_id};
  }
};
using agency_transaction_deserializer = deserializer::from_factory<agency_transaction_factory>;

/* clang-format off */

#endif  // AGENCY_OPERATION_DESERIALIZER_H

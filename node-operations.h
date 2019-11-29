//
// Created by lars on 28.11.19.
//

#ifndef AGENCY_NODE_OPERATIONS_H
#define AGENCY_NODE_OPERATIONS_H
#include "node.h"

namespace detail {

/*
 * Indicates that the value operator does not want a node_null to be created
 * if it is applied to a non existent node.
 */
struct value_operator_no_create_node {};

/*
 * Indicates that the value operator only wants to be called for nodes of the
 * specified type. Otherwise the node should be copied as is.
 */
struct value_operator_type_restricted {};  // use value_operator_only_for instead
template <typename T>
struct value_operator_only_for : value_operator_type_restricted {
  using operator_restricted_to = T;
};

template <typename F>
struct value_operator_adapter : private F {
  template <typename... S>
  explicit value_operator_adapter(S&&... s) : F(std::forward<S>(s)...) {}

  node_ptr operator()(node_ptr const& node) const {
    if (auto const& actual_node = fix_node(node); actual_node) {
      return visit_node(actual_node);
    }
    return nullptr;
  }

 private:
  template <typename E = F, std::enable_if_t<!std::is_base_of_v<value_operator_type_restricted, E>, int> = 0>
  auto visit_node(node_ptr const& node) const {
    return make_node_ptr(node->visit(self()));
  }

  template <typename E = F, std::enable_if_t<std::is_base_of_v<value_operator_type_restricted, E>, int> = 0>
  auto visit_node(node_ptr const& node) const {
    using T = typename F::operator_restricted_to;

    return node->visit(
        visitor{[this](T const& v) { return make_node_ptr(self()(v)); },
                [node](auto const&) { return node; }});
  }

  template <typename E = F, std::enable_if_t<std::is_base_of_v<value_operator_no_create_node, E>, int> = 0>
  [[nodiscard]] node_ptr const& fix_node(node_ptr const& node) const noexcept {
    return node;
  }

  template <typename E = F, std::enable_if_t<!std::is_base_of_v<value_operator_no_create_node, E>, int> = 0>
  [[nodiscard]] node_ptr const& fix_node(node_ptr const& node) const noexcept {
    return node::node_or_null(node);
  }

  F& self() { return *static_cast<F*>(this); }
  F const& self() const { return *static_cast<F const*>(this); }
};

}  // namespace detail

struct increment_value_operator {
  double delta = 1.0;

  increment_value_operator() noexcept = default;
  explicit increment_value_operator(double delta) : delta(delta) {}

  node_value<double> operator()(node_value<double> const& v) const noexcept {
    return node_value<double>{v.value + delta};
  }

  template <typename T>
  node_value<double> operator()(T const&) const noexcept {
    return node_value<double>{delta};
  }
};

struct push_value_operator {
  node_ptr node;

  explicit push_value_operator(node_ptr node) : node(std::move(node)) {}

  node_array operator()(node_array const& array) const noexcept {
    return array.push(node);
  }

  template <typename T>
  node_array operator()(T const&) const noexcept {
    return node_array{node};
  }
};

struct prepend_value_operator {
  node_ptr node;

  explicit prepend_value_operator(node_ptr node) : node(std::move(node)) {}

  node_array operator()(node_array const& array) const noexcept {
    return array.prepend(node);
  }

  template <typename T>
  node_array operator()(T const&) const noexcept {
    return node_array{node};
  }
};

struct pop_value_operator : detail::value_operator_no_create_node,
                            detail::value_operator_only_for<node_array> {
  node_value_variant operator()(node_array const& array) const noexcept {
    return array.pop();
  }
};

struct shift_value_operator : detail::value_operator_no_create_node,
                              detail::value_operator_only_for<node_array> {
  node_array operator()(node_array const& array) const noexcept {
    return array.shift();
  }
};

using prepend_operator = detail::value_operator_adapter<prepend_value_operator>;
using push_operator = detail::value_operator_adapter<push_value_operator>;
using pop_operator = detail::value_operator_adapter<pop_value_operator>;
using shift_operator = detail::value_operator_adapter<shift_value_operator>;
using increment_operator = detail::value_operator_adapter<increment_value_operator>;

struct remove_operator {
  node_ptr operator()(node_ptr const&) const noexcept { return nullptr; }
};

struct set_operator {
  node_ptr node;

  explicit set_operator(node_ptr node) : node(std::move(node)) {}

  node_ptr const& operator()(node_ptr const&) const { return node; }
};

#endif  // AGENCY_NODE_OPERATIONS_H

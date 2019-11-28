//
// Created by lars on 28.11.19.
//

#ifndef AGENCY_NODE_OPERATIONS_H
#define AGENCY_NODE_OPERATIONS_H
#include "node.h"

namespace detail {

template <typename F>
struct value_operator_adapter : private F {
  template <typename... S>
  explicit value_operator_adapter(S&&... s) : F(std::forward<S>(s)...) {}

  node_ptr operator()(node_ptr const& node) const {
    return make_node_ptr(node::node_or_null(node)->visit(self()));
  }

 private:
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

using increment_operator = detail::value_operator_adapter<increment_value_operator>;

struct remove_operator {
  node_ptr operator()(node_ptr const&) const noexcept { return nullptr; }
};

struct set_operator {
  node_ptr node;

  explicit set_operator(node_ptr node) : node(std::move(node)) {}

  node_ptr const& operator()(node_ptr const&) const { return node; }
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

struct pop_value_operator {

  node_array operator()(node_array const& array) const noexcept {
    return array.pop();
  }

  template <typename T>
  T const& operator()(T const& v) const noexcept {
    // pop on a non-array value does nothing
    return v;
  }
};

struct shift_value_operator {

  node_array operator()(node_array const& array) const noexcept {
    return array.shift();
  }

  template <typename T>
  T const& operator()(T const& v) const noexcept {
    // pop on a non-array value does nothing
    return v;
  }
};

using push_operator = detail::value_operator_adapter<push_value_operator>;
using pop_operator = detail::value_operator_adapter<pop_value_operator>;

#endif  // AGENCY_NODE_OPERATIONS_H

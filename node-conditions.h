#ifndef AGENCY_NODE_CONDITIONS_H
#define AGENCY_NODE_CONDITIONS_H
#include "node.h"

namespace detail {

/*
 * The condition is called only for non nullptr nodes. If the node is nullptr
 * the default action is executed. What this default action is can be described
 * using `condition_default_value` or others.
 */
struct condition_non_nullptr_only {};

/*
 * The condition is called only for a node having a specific type. Otherwise
 * the default action is executed. This implies `condition_non_nullptr_only`
 */
struct value_condition_type_restricted : condition_non_nullptr_only {};
template <typename T>
struct value_condition_only_for : value_condition_type_restricted {
  using condition_restricted_to = T;
};

/*
 * Returns the specified default value as default action.
 */
struct condition_default_value_specified {};
template <bool B>
struct condition_default_value : condition_default_value_specified {
  constexpr static bool default_value = B;
};

/*
 * Inverts the return value of the non-default action.
 */
struct condition_invert {};

template <typename F>
struct condition_helper : F {
  using condition_base = F;

  using F::F;

  bool operator()(node_ptr const& node) const {
    if (node) {
      return invert_if_required(visit_node(node));
    }
    return return_default();
  }

 private:
  template <typename E = F, std::enable_if_t<std::is_base_of_v<condition_default_value_specified, E>, int> = 0>
  [[nodiscard]] bool return_default() const noexcept {
    return F::default_value;
  }

  template <typename E = F, std::enable_if_t<std::negation_v<std::is_base_of<condition_default_value_specified, E>>, int> = 0>
  [[nodiscard]] bool return_default() const noexcept {
    return false;
  }

  template <typename E = F, std::enable_if_t<std::is_base_of_v<condition_invert, E>, int> = 0>
  [[nodiscard]] bool invert_if_required(bool v) const noexcept {
    return not v;
  }

  template <typename E = F, std::enable_if_t<std::negation_v<std::is_base_of<condition_invert, E>>, int> = 0>
  [[nodiscard]] bool invert_if_required(bool value) const noexcept {
    return value;
  }

  template <typename E = F, std::enable_if_t<std::is_base_of_v<value_condition_type_restricted, E>, int> = 0>
  auto visit_node(node_ptr const& node) const {
    using T = typename F::condition_restricted_to;

    return node->visit(visitor{[this](T const& v) { return F::operator()(v); },
                               [this](auto const&) { return return_default(); }});
  }

  template <typename E = F, std::enable_if_t<std::negation_v<std::is_base_of<value_condition_type_restricted, E>>, int> = 0>
  auto visit_node(node_ptr const& node) const {
    return F::operator()(node);
  }

  F& self() { return *static_cast<F*>(this); }
  F const& self() const { return *static_cast<F const*>(this); }
};

template <typename T, typename E = typename T::condition_base>
struct insert_condition_invert_trait : E, detail::condition_invert {
  using E ::E;
};

template <typename T>
struct not_condition_adapter : condition_helper<insert_condition_invert_trait<T>> {
  using E = condition_helper<insert_condition_invert_trait<T>>;
  using E::E;
};

}  // namespace detail

struct value_equals_condition : detail::condition_default_value<false> {
  node_ptr node;
  explicit value_equals_condition(node_ptr node) : node(std::move(node)) {}

  bool operator()(node_ptr const& other) const noexcept {
    return *node == *other;
  }
};

struct value_in_condition : detail::condition_default_value<false>,
                            detail::value_condition_only_for<node_array> {
  node_ptr node;
  explicit value_in_condition(node_ptr node) : node(std::move(node)) {}

  bool operator()(node_array const& array) const noexcept {
    return array.contains(node);
  }
};

struct value_is_array_condition : detail::condition_default_value<false>,
                                  detail::value_condition_only_for<node_array> {
  bool operator()(node_array const& array) const noexcept { return true; }
};

using in_condition = detail::condition_helper<value_in_condition>;
using not_in_condition = detail::not_condition_adapter<in_condition>;
using equal_condition = detail::condition_helper<value_equals_condition>;
using not_equal_condition = detail::not_condition_adapter<equal_condition>;
using is_array_condition = detail::condition_helper<value_is_array_condition>;

struct is_empty_condition {
  bool inverted;
  explicit is_empty_condition (bool inverted) : inverted(inverted) {}

  bool operator()(node_ptr const& node) const noexcept {
    return (node == nullptr) != inverted;
  }
};


template <typename F>
struct fold_adapter_and : F {
  using F::F;

  bool operator()(node_ptr const& node, bool value) const {
    return F::operator()(node) && value;
  }
};

#endif  // AGENCY_NODE_CONDITIONS_H

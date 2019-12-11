//
// Created by lars on 28.11.19.
//
#include <cassert>
#include <exception>

#include "helper-strings.h"
#include "node.h"

#include "immer/flex_vector_transient.hpp"
#include "immer/map_transient.hpp"

#include "velocypack/Builder.h"
#include "velocypack/Iterator.h"
#include "velocypack/Slice.h"

node_ptr node_array::get_impl(std::string const& key) const noexcept {
  auto parsed_value = string_to_number<std::size_t>(key);
  if (parsed_value.has_value()) {
    std::size_t i = parsed_value.value();
    if (i >= value.size()) {
      return nullptr;
    }
    return value[i];
  }
  return nullptr;
}

node_ptr node_object::get_impl(std::string const& key) const noexcept {
  if (auto it = value.find(key); it != nullptr) {
    return *it;
  }

  return nullptr;
}

thread_local node_ptr node::null_value_node = make_node_ptr(node_null{});
thread_local node_ptr node::empty_array_node = make_node_ptr(node_array{});
thread_local node_ptr node::empty_object_node = make_node_ptr(node_object{});

node_ptr node::from_slice(arangodb::velocypack::Slice s) {
  if (s.isNumber()) {
    return make_node_ptr(node_double{s.getNumber<double>()});
  } else if (s.isString()) {
    return make_node_ptr(node_string{std::move(s.copyString())});
  } else if (s.isBool()) {
    return make_node_ptr(node_bool{s.getBool()});
  } else if (s.isObject()) {
    node_object::container_type container;
    for (auto const& member : arangodb::velocypack::ObjectIterator(s)) {
      container = container.set(member.key.copyString(), node::from_slice(member.value));
    }
    return make_node_ptr(node_object{std::move(container)});
  } else if (s.isArray()) {
    node_array::container_type container;
    auto result = container.transient();
    for (auto const& member : ArrayIterator(s)) {
      result.push_back(node::from_slice(member));
    }
    return make_node_ptr(node_array{result.persistent()});
  } else if (s.isNull()) {
    return node::null_node();
  }

  std::terminate();  // unhandled type
}

template <typename P>
struct node_visitor_get {
  P& path;

  explicit node_visitor_get(P& path) noexcept : path(path) {}

  template <typename T>
  auto operator()(node_container<T> const& c) const noexcept -> node_ptr {
    auto& [head, tail] = path;
    if (auto child = c.get(head); child != nullptr) {
      return child->get(tail);
    }
    return nullptr;
  }

  template <typename T>
  auto operator()(node_value<T> const&) const noexcept -> node_ptr {
    return nullptr;
  }
};

node_ptr node::get(immut_list<std::string> const& path) const noexcept {
  if (path.empty()) {
    return node_ptr{shared_from_this()};
  }

  return std::visit(node_visitor_get{path}, value);
}

void node_object::into_builder(Builder& builder) const {
  ObjectBuilder object_builder(&builder);
  for (auto const& member : value) {
    builder.add(Value(member.first));
    member.second->into_builder(builder);
  }
}

node_object node_object::overlay_impl(node_object const& ov) const noexcept {
  /*
   * Copy all values from the base node. If the overlay contains a member that
   * has `nullptr` as value, remove the value from the result.
   * If the key is not set in the current node, just add it.
   * Otherwise overlay the base child by the overlay child.
   */
  auto result = value;
  for (auto const& member : ov.value) {
    if (member.second == nullptr) {
      result = result.erase(member.first);
    } else {
      auto& store = result[member.first];
      if (store == nullptr) {
        result = result.set(member.first, member.second);
      } else {
        result = result.set(member.first, store->overlay(member.second));
      }
    }
  }
  return node_object{std::move(result)};
}

node_ptr node_object::set_impl(std::string const& key, node_ptr const& v) const {
  auto result = value;
  if (v == nullptr) {
    result = result.erase(key);
  } else {
    result = result.set(key, v);
  }

  return make_node_ptr(node_object{std::move(result)});
}
node_object::node_object(std::string const& key, node_ptr const& v) : value{} {
  value = value.set(key, v);
}

bool node_object::operator==(node_object const& other) const noexcept {
  auto const& left = value;
  auto const& right = other.value;
  using value_type = node_object::container_type::value_type;

  return std::equal(left.begin(), left.end(), right.begin(), right.end(),
                    [](value_type const& left, value_type const& right) {
                      return left.first == right.first && *left.second == *right.second;
                    });
}

void node_array::into_builder(Builder& builder) const {
  ArrayBuilder array_builder(&builder);
  for (auto const& member : value) {
    assert(member != nullptr);
    member->into_builder(builder);
  }
}

node_ptr node_array::set_impl(std::string const& key, node_ptr const& v) const {
  if (auto const parsed_value = string_to_number<std::size_t>(key);
      parsed_value.has_value()) {
    size_t const index = parsed_value.value();
    auto result = value.transient();

    while (index >= value.size()) {
      result.push_back(node::null_node());
    }

    result.set(index, v);

    return make_node_ptr(node_array{result.persistent()});
  } else {
    node_object::container_type result;

    for (size_t i = 0; i < value.size(); ++i) {
      result = result.set(std::to_string(i), value[i]);
    }

    result = result.set(key, v);

    return make_node_ptr(node_object{std::move(result)});
  }
}

node_array node_array::overlay_impl(node_array const& ov) const noexcept {
  auto result = value.transient();
  while (ov.value.size() > result.size()) {
    result.push_back(node::null_node());
  }

  // possible values:
  // null - insert null into array
  // nullptr - do not override
  for (size_t i = 0; i < ov.value.size(); ++i) {
    auto const& v = ov.value[i];
    if (v != nullptr) {
      result.set(i, v);
    }
  }

  return node_array{result.persistent()};
}

node_array::node_array(node_ptr const& node) : value{node} {}

node_array node_array::prepend(node_ptr const& node) const {
  return node_array{value.push_front(node)};
}

node_array node_array::pop() const {
  if (value.empty()) {
    return node_array{value};
  }

  return node_array{value.take(value.size() - 1)};
}

node_array node_array::shift() const {
  if (value.empty()) {
    return node_array{value};
  }

  return node_array{value.drop(1)};
}

node_array node_array::push(node_ptr const& node) const {
  return node_array{value.push_back(node)};
}

bool node_array::operator==(node_array const& other) const noexcept {
  auto const& left = value;
  auto const& right = other.value;

  return std::equal(left.begin(), left.end(), right.begin(), right.end(),
                    [](node_ptr const& left, node_ptr const& right) {
                      return *left == *right;
                    });
}

node_array node_array::erase(node_ptr const& node) const {
  auto it = std::find_if(value.begin(), value.end(), [&node](node_ptr const& current) {
    return *node == *current;
  });
  if (it == value.end()) {
    return node_array{value};
  }
  auto pos = std::distance(value.begin(), it);
  return node_array{value.erase(pos)};
}

bool node_array::contains(node_ptr const& needle) const noexcept {
  if (needle == nullptr) {
    return false;
  }

  return std::any_of(value.begin(), value.end(),
                     [&needle](node_ptr const& node) { return *needle == *node; });
}

void node::into_builder(Builder& builder) const {
  std::visit([&](auto const& v) { v.into_builder(builder); }, value);
}

struct node_overlay_visitor {
  node_ptr const& ov;

  /*
   * For container types, do overlay on the underlying container
   */
  template <typename T>
  node_ptr operator()(node_container<T> const& base, node_container<T> const& overlay) const
      noexcept(noexcept(base.overlay(overlay))) {
    return make_node_ptr(base.overlay(overlay));
  }

  /*
   * Value types are simple replaces by the overlay value.
   */
  template <typename T>
  node_ptr operator()(node_value<T> const& base, node_value<T> const& overlay) const
      noexcept {
    return ov;
  }

  /*
   * If the types are different we always take the overlay value.
   */
  template <typename T, typename S,
            typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, std::decay_t<S>>>>
  auto operator()(T const&, S const&) const noexcept {
    return ov;
  }
};

node_ptr node::overlay(node_ptr const& ov) const {
  return std::visit(node_overlay_visitor{ov}, value, ov->value);
}

node_ptr const& node::node_or_null(node_ptr const& node) {
  if (nullptr == node) {
    return null_node();
  }
  return node;
}

template <typename P, typename N>
struct node_set_visitor {
  P& path;
  N& node;

  node_set_visitor(P& path, N& node) : path(path), node(node) {}

  template <typename T>
  auto operator()(node_container<T> const& c) const -> node_ptr {
    auto& [head, tail] = path;
    // head and tail are _not_ variables but local name bindings and
    // thus can not be captured by a lambda, except like doing so:
    auto new_child = [&, head = head, tail = tail]() {
      if (auto child = c.get(head); child != nullptr) {
        return child->set(tail, node);
      }
      return node::node_at_path(tail, node);
    }();

    return c.set(head, new_child);
  }

  template <typename T>
  auto operator()(node_value<T> const&) const -> node_ptr {
    return node::node_at_path(path, node);
  }
};

node_ptr node::set(const node::path_slice& path, node_ptr const& node) const {
  if (path.empty()) {
    return node;
  }

  return std::visit(node_set_visitor{path, node}, value);
}

node_ptr node::node_at_path(immut_list<std::string> const& path, node_ptr const& node) {
  if (path.empty()) {
    return node;
  }

  auto& [head, tail] = path;

  return make_node_ptr(node_object{head, node_at_path(tail, node)});
}

node_ptr node::modify(std::vector<std::pair<path_slice, modify_operation>> const& operations) const {
  auto curNode = node_ptr{shared_from_this()};

  for (auto const& it : operations) {
    auto& [path, op] = it;
    curNode = curNode->set(path, op(get(path)));
  }

  return curNode;
}

template <typename L>
node_ptr node::extract(const L& list) const {
  // This is a possible implementation, but not optimized
  auto node = empty_object();
  for (auto const& path : list) {
    node = node->set(path, get(path));
  }
  return node;
}

std::ostream& operator<<(std::ostream& os, node const& n) {
  // let keep things simple
  arangodb::velocypack::Builder builder;
  n.into_builder(builder);
  os << builder.toJson();
  return os;
}

template <typename T>
bool node_container<T>::operator==(const node_container<T>& other) const noexcept {
  return node_type<T>::self() == other.self();
}

template <typename T>
bool node_container<T>::operator!=(const node_container<T>& other) const noexcept {
  return !(node_type<T>::self() == other.self());
}

bool operator==(node_ptr const& other, std::nullptr_t) {
  return !other.operator bool();
}
bool operator==(std::nullptr_t, node_ptr const& other) {
  return !other.operator bool();
}
bool operator!=(node_ptr const& other, std::nullptr_t) {
  return other.operator bool();
}
bool operator!=(std::nullptr_t, node_ptr const& other) {
  return other.operator bool();
}

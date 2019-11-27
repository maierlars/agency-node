#include <iostream>
#include <map>
#include <memory>
#include <utility>
#include <variant>
#include <vector>

#include "helper.h"

template <typename K, typename V>
using immut_map = std::map<K, V>;
template <typename T>
using immut_vector = std::map<std::size_t, T>;

/*
  interface von node_object (node_array):

  // Wenn value == nullptr: Lösche key
  node_object set(std::string key, std::shared_ptr<node const> value) const;

*/

struct node;

template <typename T>
struct node_value {
  T value;
  explicit node_value(T&& t) : value(std::forward<T>(t)) {}
  void into_builder(Builder& builder) const { builder.add(Value(value)); }
};

template <typename T>
struct node_type {
  using self_type = T;

  [[nodiscard]] T& self() noexcept { return *static_cast<T*>(this); }
  [[nodiscard]] T const& self() const noexcept {
    return *static_cast<T const*>(this);
  }
};

using node_string = node_value<std::string>;
using node_double = node_value<double>;
using node_bool = node_value<bool>;

template <typename T>
struct node_container : public node_type<T> {
  [[nodiscard]] std::shared_ptr<node const> get(std::string const& key) const
      noexcept(std::is_nothrow_invocable_v<decltype(&T::get_impl), T, std::string const&>) {
    return node_type<T>::self().get_impl(key);
  };

  [[nodiscard]] T set(std::string const& key, std::shared_ptr<node const> const& v) const
      noexcept(std::is_nothrow_invocable_v<decltype(&T::set_impl), T, std::string const&,
                                           std::shared_ptr<node const> const&>) {
    return node_type<T>::self().set_impl(key, v);
  };

  [[nodiscard]] T overlay(node_container<T> const& t) const
      noexcept(std::is_nothrow_invocable_v<decltype(&T::overlay_impl), T, T const&>) {
    return node_type<T>::self().overlay_impl(t.self());
  }
};

template <typename T>
constexpr const bool node_container_is_nothrow_get =
    std::is_nothrow_invocable_v<decltype(&T::get), T, std::string const&>;

template <typename T>
constexpr const bool node_container_is_nothrow_overlay =
    std::is_nothrow_invocable_v<decltype(&T::overlay), node_container<T>, node_container<T> const&>;

struct node_array final : public node_container<node_array> {
  using container_type = std::vector<std::shared_ptr<node const>>;
  container_type value;

  node_array() noexcept(std::is_nothrow_constructible_v<container_type>) = default;
  explicit node_array(container_type value) : value(std::move(value)) {}

  node_array(node_array const&) = delete;
  node_array& operator=(node_array const&) = delete;

  node_array(node_array&&) noexcept(std::is_nothrow_move_constructible_v<container_type>) = default;
  node_array& operator=(node_array&&) noexcept(std::is_nothrow_move_assignable_v<container_type>) = default;

  [[nodiscard]] std::shared_ptr<node const> get_impl(std::string const& key) const noexcept;

  [[nodiscard]] node_array overlay_impl(node_array const& ov) const noexcept {
    std::terminate();
  }

  [[nodiscard]] node_array set_impl(std::string const& key,
                                    std::shared_ptr<node const> const& v) const {
    std::terminate();
  }

  void into_builder(Builder& builder) const;
};

static_assert(node_container_is_nothrow_get<node_array>);
static_assert(node_container_is_nothrow_overlay<node_array>);

struct node_object final : public node_container<node_object> {
  using container_type = std::map<std::string, std::shared_ptr<node const>>;

  container_type value;

  node_object() noexcept(std::is_nothrow_constructible_v<container_type>) = default;
  explicit node_object(container_type value) : value(std::move(value)) {}
  explicit node_object(std::string const& key, std::shared_ptr<node const> const& v) {
    value[key] = v;
  }

  node_object(node_object const&) = delete;
  node_object& operator=(node_object const&) = delete;

  node_object(node_object&&) noexcept(std::is_nothrow_move_constructible_v<container_type>) = default;
  node_object& operator=(node_object&&) noexcept(std::is_nothrow_move_assignable_v<container_type>) = default;

  [[nodiscard]] node_object overlay_impl(node_object const& ov) const noexcept;

  [[nodiscard]] std::shared_ptr<node const> get_impl(std::string const& key) const noexcept;

  [[nodiscard]] node_object set_impl(std::string const& key,
                                     std::shared_ptr<node const> const& v) const;

  void into_builder(Builder& builder) const;
};

static_assert(node_container_is_nothrow_get<node_object>);
static_assert(node_container_is_nothrow_overlay<node_object>);

std::shared_ptr<node const> node_array::get_impl(std::string const& key) const noexcept {
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

std::shared_ptr<node const> node_object::get_impl(std::string const& key) const noexcept {
  if (auto it = value.find(key); it != value.end()) {
    return it->second;
  }

  return nullptr;
}

struct node : public std::enable_shared_from_this<node> {
 private:
  std::variant<node_string, node_double, node_bool, node_array, node_object> value;

 public:
  using path_slice = immut_list<std::string>;

  node(node const&) = delete;
  node(node&&) = delete;
  node& operator=(node const&) = delete;
  node& operator=(node&&) = delete;

  /*
   * TODO this constructor needs to be public because of make_shared. Maybe
   * we can add some friends to make this constructor private. (and only allow
   * shared_ptr nodes)
   */
  template <typename T>
  explicit node(T&& v) : value(std::forward<T>(v)){};

  template <typename F>
  auto visit(F&& f) {
    return std::visit(std::forward<F>(f), value);
  }

  template <typename T>
  std::shared_ptr<node const> static from_buffer_ptr(Buffer<T> const& ptr);
  std::shared_ptr<node const> static from_slice(Slice slice);
  std::shared_ptr<node const> static empty_node() {
    return std::make_shared<node const>(node_object{});
  };

  std::shared_ptr<node const> static node_at_path(immut_list<std::string> const& path,
                                                  std::shared_ptr<node const> const& node);

  template <typename T>
  std::shared_ptr<node const> static value_node(T&& t) {
    return std::make_shared<node const>(node_value<T>{std::forward<T>(t)});
  }

  std::shared_ptr<node const> set(path_slice const& path,
                                  std::shared_ptr<node const> const& node) const;
  bool has(path_slice const& path) const { return get(path) != nullptr; }
  std::shared_ptr<node const> get(path_slice const& path) const noexcept;

  std::shared_ptr<node const> remove(path_slice const& path) const {
    return set(path, nullptr);
  }

  /*
   * Extract returns a node that contains all the subtrees of the specified
   * nodes.
   */
  template <typename L>
  std::shared_ptr<node const> extract(L const& list) const {
    // This is a possible implementation, but not optimized
    auto node = empty_node();
    for (auto const& path : list) {
      node = node->set(path, get(path));
    }
    return node;
  }

  /*
   * Returns the underlying node overlayed by `ov`. i.e. if a value is present
   * in ov it is replaced. Here `nullptr` indicated `deleted`.
   */
  std::shared_ptr<node const> overlay(std::shared_ptr<node const> const& ov) const;

  void into_builder(Builder& builder) const;
};

template <typename T>
std::shared_ptr<node const> node::from_buffer_ptr(const Buffer<T>& ptr) {
  return node::from_slice(Slice(ptr.data()));
}

std::shared_ptr<node const> node::from_slice(Slice slice) {
  if (slice.isNumber()) {
    return std::make_shared<node const>(node_double{slice.getNumber<double>()});
  } else if (slice.isString()) {
    return std::make_shared<node const>(node_string{std::move(slice.copyString())});
  } else if (slice.isBool()) {
    return std::make_shared<node const>(node_bool{slice.getBool()});
  } else if (slice.isObject()) {
    node_object::container_type container;
    for (auto const& member : ObjectIterator(slice)) {
      container[member.key.copyString()] = node::from_slice(member.value);
    }
    return std::make_shared<node const>(node_object{std::move(container)});
  } else if (slice.isArray()) {
    node_array::container_type container;
    container.reserve(slice.length());
    for (auto const& member : ArrayIterator(slice)) {
      container.push_back(node::from_slice(member));
    }
    return std::make_shared<node const>(node_array{std::move(container)});
  } else {
    return nullptr;
  }
}

template <typename P>
struct node_visitor_get {
  P& path;

  explicit node_visitor_get(P& path) noexcept : path(path) {}

  template <typename T>
  auto operator()(node_container<T> const& c) const noexcept
      -> std::shared_ptr<node const> {
    auto& [head, tail] = path;
    if (auto child = c.get(head); child != nullptr) {
      return child->get(tail);
    }
    return nullptr;
  }

  template <typename T>
  auto operator()(node_value<T> const&) const noexcept -> std::shared_ptr<node const> {
    return nullptr;
  }
};

std::shared_ptr<node const> node::get(immut_list<std::string> const& path) const noexcept {
  if (path.empty()) {
    return shared_from_this();
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
  container_type result{value};
  for (auto const& member : ov.value) {
    if (member.second == nullptr) {
      result.erase(member.first);
    } else {
      auto& store = result[member.first];
      if (store == nullptr) {
        store = member.second;
      } else {
        store = store->overlay(member.second);
      }
    }
  }
  return node_object{result};
}

node_object node_object::set_impl(std::string const& key,
                                  std::shared_ptr<node const> const& v) const {
  container_type result{value};
  if (v == nullptr) {
    result.erase(key);
  } else {
    result[key] = v;
  }

  return node_object{std::move(result)};
}

void node_array::into_builder(Builder& builder) const {
  ArrayBuilder array_builder(&builder);
  for (auto const& member : value) {
    member->into_builder(builder);
  }
}

void node::into_builder(Builder& builder) const {
  std::visit([&](auto const& v) { v.into_builder(builder); }, value);
}

struct node_overlay_visitor {
  std::shared_ptr<node const> const& ov;

  /*
   * For container types, do overlay on the underlying container
   */
  template <typename T>
  std::shared_ptr<node const> operator()(node_container<T> const& base,
                                         node_container<T> const& overlay) const
      noexcept(noexcept(base.overlay(overlay))) {
    return std::make_shared<node>(base.overlay(overlay));
  }

  /*
   * Value types are simple replaces by the overlay value.
   */
  template <typename T>
  std::shared_ptr<node const> operator()(node_value<T> const& base,
                                         node_value<T> const& overlay) const noexcept {
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

std::shared_ptr<node const> node::overlay(std::shared_ptr<node const> const& ov) const {
  return std::visit(node_overlay_visitor{ov}, value, ov->value);
}

template <typename P, typename N>
struct node_set_visitor {
  P& path;
  N& node;

  node_set_visitor(P& path, N& node) : path(path), node(node) {}

  template <typename T>
  auto operator()(node_container<T> const& c) const
      -> std::shared_ptr<struct node const> {
    auto& [head, tail] = path;
    // head and tail are _not_ variables but local name bindings and
    // thus can not be captured by a lambda, except like doing so:
    auto new_child = [&, head = head, tail = tail]() {
      if (auto child = c.get(head); child != nullptr) {
        return child->set(tail, node);
      }
      return node::node_at_path(tail, node);
    }();

    return std::make_shared<struct node const>(std::move(c.set(head, new_child).self()));
  }

  template <typename T>
  auto operator()(node_value<T> const&) const -> std::shared_ptr<struct node const> {
    return node::node_at_path(path, node);
  }
};

std::shared_ptr<node const> node::set(const node::path_slice& path,
                                      std::shared_ptr<node const> const& node) const {
  if (path.empty()) {
    return node;
  }

  return std::visit(node_set_visitor{path, node}, value);
}

std::shared_ptr<node const> node::node_at_path(immut_list<std::string> const& path,
                                               std::shared_ptr<node const> const& node) {
  if (path.empty()) {
    return node;
  }

  auto& [head, tail] = path;

  return std::make_shared<struct node const>(node_object{head, node_at_path(tail, node)});
}

std::ostream& operator<<(std::ostream& os, node const& n) {
  // let keep things simple
  Builder builder;
  n.into_builder(builder);
  os << builder.toJson();
  return os;
}

using namespace std::string_literals;

int main(int argc, char* argv[]) {
  auto n = node::from_buffer_ptr(R"=({"key":{"hello":"world", "foo":12}})="_vpack);

  std::cout << *n << std::endl;

  auto m = node::from_buffer_ptr(R"=({"key":{"bar":12}, "foo":["blub"]})="_vpack);
  std::cout << *m << std::endl;

  std::cout << *n->overlay(m) << std::endl;

  auto p = n->set(immut_list{"key"s, "world"s}, node::value_node(12.0));

  std::cout << *p << std::endl;
}

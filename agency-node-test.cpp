#include <iostream>
#include <map>
#include <memory>
#include <utility>
#include <variant>
#include <vector>

#include "helper.h"

template <typename K, typename V> using immut_map = std::map<K, V>;
template <typename T> using immut_vector = std::map<std::size_t, T>;

/*
  interface von node_object (node_array):

  // Wenn value == nullptr: Lösche key
  node_object set(std::string key, std::shared_ptr<node const> value) const;

*/

struct node;

template <typename T> struct node_value {
  T value;

  void into_builder(Builder &builder) const { builder.add(Value(value)); }
};

struct node_string : public node_value<std::string> {};
struct node_double : public node_value<double> {};
struct node_bool : public node_value<bool> {};

template <typename T> struct node_container {
  using self_type = T;

  [[nodiscard]] T &self() noexcept { return *static_cast<T *>(this); }
  [[nodiscard]] T const &self() const noexcept {
    return *static_cast<T const *>(this);
  }

  [[nodiscard]] std::shared_ptr<node const> get(std::string const &key) const
      noexcept(
          noexcept(std::declval<T>().get_impl(std::declval<std::string>()))) {
    return self().get_impl(key);
  };

  [[nodiscard]] T overlay(node_container<T> const &t) const
      noexcept(noexcept(std::declval<T>().overlay_impl(std::declval<T>()))) {
    return self().overlay_impl(t.self());
  }
};

struct node_array final : public node_container<node_array> {
  using container_type = std::vector<std::shared_ptr<node const>>;
  container_type value;

  explicit node_array(container_type value) : value(std::move(value)) {}

  [[nodiscard]] std::shared_ptr<node const>
  get_impl(std::string const &key) const noexcept;

  [[nodiscard]] node_array overlay_impl(node_array const &ov) const noexcept {
    std::terminate();
  }

  void into_builder(Builder &builder) const;
};

struct node_object final : public node_container<node_object> {
  using container_type = std::map<std::string, std::shared_ptr<node const>>;

  container_type value;

  explicit node_object(container_type value) : value(std::move(value)) {}
  explicit node_object(std::string const &key,
                       std::shared_ptr<node const> const &v) {
    value[key] = v;
  }
  node_object() noexcept = default;

  [[nodiscard]] node_object overlay_impl(node_object const &ov) const noexcept;

  [[nodiscard]] std::shared_ptr<node const>
  get_impl(std::string const &key) const noexcept;

  void into_builder(Builder &builder) const;
};

std::shared_ptr<node const> node_array::get_impl(std::string const &key) const
    noexcept {
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

std::shared_ptr<node const> node_object::get_impl(std::string const &key) const
    noexcept {
  if (auto it = value.find(key); it != value.end()) {
    return it->second;
  }

  return nullptr;
}

struct node : public std::enable_shared_from_this<node> {
  std::variant<node_string, node_double, node_bool, node_array, node_object>
      value;

  using path_slice = immut_list<std::string>;

  std::shared_ptr<node const> get(path_slice const &path) const noexcept;

  node(node const &) = delete;
  node(node &&) = delete;
  node &operator=(node const &) = delete;
  node &operator=(node &&) = delete;

  template <typename T>
  std::shared_ptr<node const> static from_buffer_ptr(Buffer<T> const &ptr);
  std::shared_ptr<node const> static from_slice(Slice slice);
  std::shared_ptr<node const> static empty_node() {
    return std::make_shared<node const>(node_object{});
  };

  void into_builder(Builder &builder) const;

  template <typename T> explicit node(T &&v) : value(std::forward<T>(v)){};

  std::shared_ptr<node const> remove(path_slice const &path) const {
    return set(path, nullptr);
  }

  bool has(path_slice const &path) const { return get(path) != nullptr; }

  std::shared_ptr<node const> set(path_slice const &path,
                                  std::shared_ptr<node> const &node) const;

  /*
   * Extract returns a node that contains all the subtrees of the specified
   * nodes.
   */
  template <typename L>
  std::shared_ptr<node const> extract(L const &list) const {
    // This is a possible implementation, but not optimized
    auto node = empty_node();
    for (auto const &path : list) {
      node = node->set(path, get(path));
    }
    return node;
  }

  /*
   * Returns the underlying node overlayed by `ov`. i.e. if a value is present
   * in ov it is replaced. Here `nullptr` indicated `deleted`.
   */
  std::shared_ptr<node const>
  overlay(std::shared_ptr<node const> const &ov) const;

private:
  std::shared_ptr<node const> static nodeFromPath(
      immut_list<std::string> const &path, std::shared_ptr<node> const &node) {
    if (path.empty()) {
      return node;
    }

    auto &[head, tail] = path;

    return std::make_shared<struct node const>(
        node_object{head, nodeFromPath(tail, node)});
  }

  /*

  std::shared_ptr<node const> set(immut_list<std::string> const &path,
                                  std::shared_ptr<node> const &node) const {
    if (path.empty()) {
      return node;
    }

    auto [head, tail] = path;

    return std::visit(visitor{
        [&](node_object const &object) {
          auto newChild = []() {
            if (auto it = object.value.get(head); it != object.value.end()) {
              return it->set(tail, node);
            }
            return nodeFromPath(tail, node);
          }();

          return {node_object{object.value.set(head, newChild)}};
        },
        [](auto child) {
          auto newChild = nodeFromPath(path, node);
          return newChild;
        }});
  }

  std::shared_ptr<node const>
  remove(immut_list<std::string> const &path) const {
    if (path.empty()) {
      return nullptr;
    }

    auto [head, tail] = path;

    return std::visit(visitor{
        [&](node_object const &object) {
          if (auto it = object.value.get(head); it != object.value.end()) {
            auto newChild = it->remove(tail);
            TRI_ASSERT((newChild == nullptr) == tail.empty());
            return {node_object{object.value.set(head, newChild)}};
          }

          return shared_from_this();
        },
        [](auto child) { return shared_from_this(); }});
  }
  */
};

template <typename T>
std::shared_ptr<node const> node::from_buffer_ptr(const Buffer<T> &ptr) {
  return node::from_slice(Slice(ptr.data()));
}

std::shared_ptr<node const> node::from_slice(Slice slice) {
  if (slice.isNumber()) {
    return std::make_shared<node const>(node_double{slice.getNumber<double>()});
  } else if (slice.isString()) {
    return std::make_shared<node const>(
        node_string{std::move(slice.copyString())});
  } else if (slice.isBool()) {
    return std::make_shared<node const>(node_bool{slice.getBool()});
  } else if (slice.isObject()) {
    node_object::container_type container;
    for (auto const &member : ObjectIterator(slice)) {
      container[member.key.copyString()] = node::from_slice(member.value);
    }
    return std::make_shared<node const>(node_object{std::move(container)});
  } else if (slice.isArray()) {
    node_array::container_type container;
    container.reserve(slice.length());
    for (auto const &member : ArrayIterator(slice)) {
      container.push_back(node::from_slice(member));
    }
    return std::make_shared<node const>(node_array{std::move(container)});
  } else {
    return nullptr;
  }
}

template <typename P> struct node_visitor_get {
  P &path;

  explicit node_visitor_get(P &path) noexcept : path(path) {}

  template <typename T>
  auto operator()(node_container<T> const &c) const noexcept
      -> std::shared_ptr<node const> {
    auto &[head, tail] = path;
    if (auto child = c.get(head); child != nullptr) {
      return child->get(tail);
    }
    return nullptr;
  }

  template <typename T>
  auto operator()(node_value<T> const &) const noexcept
      -> std::shared_ptr<node const> {
    return nullptr;
  }
};

std::shared_ptr<node const> node::get(immut_list<std::string> const &path) const
    noexcept {
  if (path.empty()) {
    return shared_from_this();
  }

  return std::visit(node_visitor_get{path}, value);
}

void node_object::into_builder(Builder &builder) const {
  ObjectBuilder object_builder(&builder);
  for (auto const &member : value) {
    builder.add(Value(member.first));
    member.second->into_builder(builder);
  }
}

node_object node_object::overlay_impl(node_object const &ov) const noexcept {
  /*
   * Copy all values from the base node. If the overlay contains a member that
   * has `nullptr` as value, remove the value from the result.
   * If the key is not set in the current node, just add it.
   * Otherwise overlay the base child by the overlay child.
   */
  container_type result{value};
  for (auto const &member : ov.value) {
    if (member.second == nullptr) {
      result.erase(member.first);
    } else {
      auto &store = result[member.first];
      if (store == nullptr) {
        store = member.second;
      } else {
        store = store->overlay(member.second);
      }
    }
  }
  return node_object{result};
}

void node_array::into_builder(Builder &builder) const {
  ArrayBuilder array_builder(&builder);
  for (auto const &member : value) {
    member->into_builder(builder);
  }
}

void node::into_builder(Builder &builder) const {
  std::visit([&](auto const &v) { v.into_builder(builder); }, value);
}

struct node_overlay_visitor {
  std::shared_ptr<node const> const &ov;

  /*
   * For container types, do overlay on the underlying container
   */
  template <typename T>
  std::shared_ptr<node const> operator()(node_container<T> const &base,
                                         node_container<T> const &overlay) const
      noexcept(noexcept(base.overlay(overlay))) {
    return std::make_shared<node>(base.overlay(overlay));
  }

  /*
   * Value types are simple replaces by the overlay value.
   */
  template <typename T>
  std::shared_ptr<node const> operator()(node_value<T> const &base,
                                         node_value<T> const &overlay) const
      noexcept {
    return ov;
  }

  /*
   * If the types are different we always take the overlay value.
   */
  template <typename T, typename S,
            typename = std::enable_if_t<
                !std::is_same_v<std::decay_t<T>, std::decay_t<S>>>>
  auto operator()(T const &, S const &) const noexcept {
    return ov;
  }
};

std::shared_ptr<node const>
node::overlay(std::shared_ptr<node const> const &ov) const {
  return std::visit(node_overlay_visitor{ov}, value, ov->value);
}

std::shared_ptr<node const> node::set(const node::path_slice &path,
                                      std::shared_ptr<node> const &node) const {
  // TODO
  return std::shared_ptr<struct node const>();
}

std::ostream &operator<<(std::ostream &os, node const &n) {
  // let keep things simple
  Builder builder;
  n.into_builder(builder);
  os << builder.toJson();
  return os;
}

using namespace std::string_literals;

int main(int argc, char *argv[]) {

  auto n =
      node::from_buffer_ptr(R"=({"key":{"hello":"world", "foo":12}})="_vpack);

  std::cout << *n << std::endl;

  auto m =
      node::from_buffer_ptr(R"=({"key":{"bar":12}, "foo":["blub"]})="_vpack);
  std::cout << *m << std::endl;

  std::cout << *n->overlay(m);
}

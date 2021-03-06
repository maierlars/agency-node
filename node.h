#ifndef AGENCY_NODE_H
#define AGENCY_NODE_H

#include <map>
#include <variant>
#include <vector>

#include "immer/flex_vector.hpp"
#include "immer/map.hpp"

#include "velocypack/Builder.h"
#include "velocypack/Slice.h"

#include "helper-immut.h"

struct null_type {};

template <typename T>
struct node_value final {
  T value;

  explicit node_value(T&& t) : value(std::move(t)) {}
  explicit node_value(T const& t) : value(t) {}

  node_value(node_value const&) = delete;
  node_value& operator=(node_value const&) = delete;
  node_value(node_value&&) noexcept = default;
  node_value& operator=(node_value&&) noexcept = default;

  bool operator==(node_value const& other) const noexcept {
    return value == other.value;
  }
  bool operator!=(node_value const& other) const noexcept {
    return value != other.value;
  }

  void into_builder(arangodb::velocypack::Builder& builder) const {
    builder.add(Value(value));
  }
};

template <>
struct node_value<null_type> final {
  void into_builder(arangodb::velocypack::Builder& builder) const {
    builder.add(arangodb::velocypack::Slice::nullSlice());
  }
  bool operator==(node_value<null_type> const& other) const noexcept {
    return true;
  }
  bool operator!=(node_value<null_type> const& other) const noexcept {
    return false;
  }
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
using node_null = node_value<null_type>;

struct node_array;
struct node_object;

struct node;

class node_ptr : private std::shared_ptr<node const> {
 public:
  using std::shared_ptr<node const>::shared_ptr;
  using std::shared_ptr<node const>::operator*;
  using std::shared_ptr<node const>::operator->;
  using std::shared_ptr<node const>::operator=;
  using std::shared_ptr<node const>::operator bool;

  explicit node_ptr(std::shared_ptr<node const>&& r) noexcept
      : std::shared_ptr<node const>(std::move(r)){};
  explicit node_ptr(std::shared_ptr<node const> const& r) noexcept
      : std::shared_ptr<node const>(r){};
};

bool operator==(node_ptr const& other, std::nullptr_t);
bool operator==(std::nullptr_t, node_ptr const& other);
bool operator!=(node_ptr const& other, std::nullptr_t);
bool operator!=(std::nullptr_t, node_ptr const& other);

template <typename... T>
auto inline make_node_ptr(T&&... t) -> node_ptr {
  return node_ptr{std::make_shared<node const>(std::forward<T>(t)...)};
}

using node_value_variant =
    std::variant<node_string, node_double, node_bool, node_array, node_object, node_null>;

template <typename T>
struct node_container : public node_type<T> {
  template <typename K>
  [[nodiscard]] node_ptr get(K&& key) const
      noexcept(std::is_nothrow_invocable_v<decltype(&T::get_impl), T, decltype(std::forward<K>(key))>) {
    return node_type<T>::self().get_impl(std::forward<K>(key));
  };

  [[nodiscard]] node_ptr set(std::string const& key, node_ptr const& v) const
      noexcept(std::is_nothrow_invocable_v<decltype(&T::set_impl), T, std::string const&, node_ptr const&>) {
    return node_type<T>::self().set_impl(key, v);
  };

  [[nodiscard]] T overlay(node_container<T> const& t) const
      noexcept(std::is_nothrow_invocable_v<decltype(&T::overlay_impl), T, T const&>) {
    return node_type<T>::self().overlay_impl(t.self());
  }

  // TODO those implementations are wrong since std::shared_ptr compare the
  // pointer values instead of the objects.
  bool operator==(node_container<T> const& other) const noexcept;
  bool operator!=(node_container<T> const& other) const noexcept;
};

template <typename T>
constexpr const bool node_container_is_nothrow_get =
    std::is_nothrow_invocable_v<decltype(&T::template get<std::string const&>), T, std::string const&>;

template <typename T>
constexpr const bool node_container_is_nothrow_overlay =
    std::is_nothrow_invocable_v<decltype(&T::overlay), node_container<T>, node_container<T> const&>;

struct node_array final : public node_container<node_array> {
  using container_type = immer::flex_vector<node_ptr>;
  container_type value;

  node_array() noexcept(std::is_nothrow_constructible_v<container_type>) = default;
  explicit node_array(container_type value) : value(std::move(value)) {}
  explicit node_array(node_ptr const&);

  node_array(node_array const&) = delete;
  node_array& operator=(node_array const&) = delete;
  node_array(node_array&&) noexcept(std::is_nothrow_move_constructible_v<container_type>) = default;
  node_array& operator=(node_array&&) noexcept(std::is_nothrow_move_assignable_v<container_type>) = default;

  [[nodiscard]] node_ptr get_impl(std::string const& key) const noexcept;
  [[nodiscard]] node_array overlay_impl(node_array const& ov) const noexcept;
  [[nodiscard]] node_ptr set_impl(std::string const& key, node_ptr const& v) const;
  void into_builder(arangodb::velocypack::Builder& builder) const;

  [[nodiscard]] node_array push(node_ptr const&) const;
  [[nodiscard]] node_array prepend(node_ptr const&) const;
  [[nodiscard]] node_array pop() const;
  [[nodiscard]] node_array shift() const;
  [[nodiscard]] node_array erase(node_ptr const& value) const;

  [[nodiscard]] bool contains(node_ptr const& value) const noexcept;

  [[nodiscard]] bool operator==(node_array const&) const noexcept;
};

struct node_object final : public node_container<node_object> {
  using container_type = immer::map<std::string, node_ptr>;

  container_type value;

  node_object() noexcept(std::is_nothrow_constructible_v<container_type>) = default;
  explicit node_object(container_type value) : value(std::move(value)) {}
  explicit node_object(std::string const& key, node_ptr const& v);

  node_object(node_object const&) = delete;
  node_object& operator=(node_object const&) = delete;
  node_object(node_object&&) noexcept(std::is_nothrow_move_constructible_v<container_type>) = default;
  node_object& operator=(node_object&&) noexcept(std::is_nothrow_move_assignable_v<container_type>) = default;

  [[nodiscard]] node_object overlay_impl(node_object const& ov) const noexcept;
  [[nodiscard]] node_ptr get_impl(std::string const& key) const noexcept;
  [[nodiscard]] node_ptr set_impl(std::string const& key, node_ptr const& v) const;
  void into_builder(arangodb::velocypack::Builder& builder) const;

  [[nodiscard]] bool operator==(node_object const&) const noexcept;
};

static_assert(node_container_is_nothrow_get<node_array>);
static_assert(node_container_is_nothrow_overlay<node_array>);
static_assert(node_container_is_nothrow_get<node_object>);
static_assert(node_container_is_nothrow_overlay<node_object>);

struct node : public std::enable_shared_from_this<node> {
 private:
  node_value_variant value;

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
  auto visit(F&& f) const {
    return std::visit(std::forward<F>(f), value);
  }

  template <typename T>
  static node_ptr from_buffer_ptr(const arangodb::velocypack::Buffer<T>& ptr) {
    return from_slice(arangodb::velocypack::Slice(ptr.data()));
  }

  static node_ptr from_slice(arangodb::velocypack::Slice s);
  static node_ptr const& empty_object() { return empty_object_node; };
  static node_ptr const& null_node() { return null_value_node; }
  static node_ptr const& empty_array() { return empty_array_node; }
  static node_ptr const& node_or_null(node_ptr const& node);

  node_ptr static node_at_path(immut_list<std::string> const& path, node_ptr const& node);

  template <typename T>
  node_ptr static value_node(T&& t) {
    return make_node_ptr(node_value<T>{std::forward<T>(t)});
  }

  node_ptr set(path_slice const& path, node_ptr const& node) const;
  node_ptr get(path_slice const& path) const noexcept;

  bool has(path_slice const& path) const { return get(path) != nullptr; }
  node_ptr remove(path_slice const& path) const { return set(path, nullptr); }

  using transformation = std::function<node_ptr(node_ptr const&)>;
  using transform_action = std::pair<path_slice, transformation>;

  /*
   * Applies the given list of actions and returns the resulting node.
   * If one path is the prefix of another the result is undefined.
   */
  node_ptr transform(std::vector<transform_action> const& operations) const;

  template <typename T>
  using fold_operator = std::function<T(node_ptr const&)>;
  template <typename T>
  using fold_action = std::pair<path_slice, fold_operator<T>>;

  template <typename T>
  [[deprecated]] T fold(std::vector<fold_action<T>> const& actions,
         std::function<T(T const&, T const&)> f, T i = T()) const {
    for (fold_action<T> const& action : actions) {
      i = f(i, action.second(get(action.first)));
    }
    return i;
  };

  /*
   * Extract returns a node that contains all the subtrees of the specified
   * nodes.
   */
  template <typename L>
  node_ptr extract(L const& list) const;

  /*
   * Returns the underlying node overlayed by `ov`. i.e. if a value is present
   * in ov it is replaced. Here `nullptr` indicated `deleted`.
   */
  node_ptr overlay(node_ptr const& ov) const;

  void into_builder(arangodb::velocypack::Builder& builder) const;

  bool operator==(node const& n) const noexcept { return value == n.value; }
  bool operator!=(node const& n) const noexcept { return value != n.value; }

 private:
  static thread_local node_ptr null_value_node;
  static thread_local node_ptr empty_array_node;
  static thread_local node_ptr empty_object_node;
};

std::ostream& operator<<(std::ostream& os, node const& n);

#endif  // AGENCY_NODE_H

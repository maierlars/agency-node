#include <iostream>
#include <map>
#include <memory>

#include "helper-immut.h"

#include "node-conditions.h"
#include "node-operations.h"
#include "node.h"
#include "store.h"

/*
template <typename K, typename V>
using immut_map = std::map<K, V>;
template <typename T>
using immut_vector = std::map<std::size_t, T>;
*/

/*
node::modify_operation modify_operation_from_slice(arangodb::velocypack::Slice s) {
  if (s.isObject()) {
  } else {
    // old style set is not supported
    std::terminate();
    return set_operator{node::from_slice(s)};
  }
}

std::vector<node::modify_action> modify_action_list_from_slice(arangodb::velocypack::Slice s) {
  std::vector<node::modify_action> result;
  for (auto const& action : ObjectIterator(s)) {
    result.emplace_back(action.key.copyString(), modify_operation_from_slice(action.value));
  }

  return result;
}*/
using namespace std::string_literals;

void node_test() {
  auto n = node::from_buffer_ptr(R"=({"key":{"hello":"world", "foo":12}})="_vpack);

  std::cout << *n << std::endl;

  auto m = node::from_buffer_ptr(R"=({"key":{"bar":12}, "foo":["blub"]})="_vpack);
  std::cout << *m << std::endl;

  std::cout << *n->overlay(m) << std::endl;

  auto p = n->set(immut_list{"key"s, "world"s}, node::value_node(12.0));

  std::cout << *p << std::endl;

  auto n1 = n->modify({
      {immut_list{"key"s, "hello"s},
       [](node_ptr const&) { return node::value_node(7.0); }},
      {immut_list{"key"s, "foo"s},
       [](node_ptr const&) { return node::value_node("42"s); }},
      {immut_list{"key"s, "foox"s, "bar"s},
       [](node_ptr const&) { return node::value_node(false); }},
  });

  std::cout << *n << std::endl;
  std::cout << *n1 << std::endl;

  auto m1 = m->modify({
      {immut_list{"foo"s, "0"s, "bar"s},
       [](node_ptr const&) { return node::value_node(false); }},
  });
  std::cout << "m1 " << *m1 << std::endl;
  auto m2 = m->modify({
      {immut_list{"foo"s, "x"s, "bar"s},
       [](node_ptr const&) { return node::value_node(false); }},
  });
  std::cout << "m2 " << *m2 << std::endl;
  auto m3 = m->modify({
      {immut_list{"foo"s, "3"s, "bar"s},
       [](node_ptr const&) { return node::value_node(false); }},
  });
  std::cout << "m3 " << *m3 << std::endl;

  auto q = node::from_buffer_ptr(R"=({"foo":12, "bar":"hello"})="_vpack);

  auto q1 = q->modify({
      {immut_list{"foo"s}, increment_operator{15.0}},
      {immut_list{"bar"s}, increment_operator{}},
  });
  std::cout << *q1 << std::endl;

  auto q2 = q->modify({
      {immut_list{"foo"s}, remove_operator{}},
      {immut_list{"bar"s}, increment_operator{}},
  });
  std::cout << *q2 << std::endl;

  auto q3 = q->modify({
      {immut_list{"foo"s}, push_operator{n}},
  });
  std::cout << *q3 << std::endl;

  auto q4 = q->modify({
      {immut_list{"baz"s}, push_operator{n}},
  });
  std::cout << *q4 << std::endl;

  std::cout << "push pop prepend shift" << std::endl;

  auto r = node::from_buffer_ptr(R"=({"key":[1, 2, 3, 4, 5, 6]})="_vpack);
  std::cout << *r << std::endl;

  auto r1 = r->modify({
      {immut_list{"baz"s}, pop_operator{}},  // pop on baz should not create a node_null
      {immut_list{"key"s}, pop_operator{}},
  });
  std::cout << *r1 << std::endl;

  auto r2 = r->modify({
      {immut_list{"key"s}, shift_operator{}},
  });
  std::cout << *r2 << std::endl;

  auto r3 = r->modify({
      {immut_list{"key"s}, prepend_operator{node::value_node(12.0)}},
  });
  std::cout << *r3 << std::endl;

  auto fooBar = node::from_buffer_ptr(R"=({"foo": "bar"})="_vpack);
  auto a = node::from_buffer_ptr(
      R"=({"foo": ["a", "b", [null], 1, 2, {"foo": "bar"}, "a", [], "b", [], {"foo": "baz"}, {}, 1, 2]})="_vpack);
  std::cout << *a->modify({{immut_list{"foo"s}, erase_operator{node::value_node(1.0)}}})
            << std::endl;
  std::cout << *a->modify({{immut_list{"foo"s}, erase_operator{node::value_node("a"s)}}})
            << std::endl;
  std::cout << *a->modify({{immut_list{"foo"s}, erase_operator{node::empty_array()}}})
            << std::endl;
  std::cout << *a->modify({{immut_list{"foo"s}, erase_operator{node::empty_object()}}})
            << std::endl;
  std::cout << *a->modify({{immut_list{"foo"s}, erase_operator{fooBar}}}) << std::endl;

  std::cout << "fold" << std::endl;

  auto s = node::from_buffer_ptr(R"=({"key":{"hello":"world", "foo":12}})="_vpack);

  bool f1 = s->fold<bool>(
      {
          {immut_list{"key"s, "hello"s}, equal_condition{node::value_node("world"s)}},
          {immut_list{"key"s, "foo"s}, equal_condition{node::value_node(12.0)}},
      },
      std::logical_and{}, true);
  std::cout << std::boolalpha << f1 << std::endl;

  bool f2 = s->fold<bool>(
      {
          {immut_list{"key"s, "hello"s}, not_equal_condition{node::value_node("worl2"s)}},
          {immut_list{"key"s, "foobaz"s}, not_equal_condition{node::value_node(12.0)}},
      },
      std::logical_and{}, true);
  std::cout << std::boolalpha << f2 << std::endl;

  bool f5 = r->fold<bool>(
      {
          {immut_list{"key-non-existing"s}, in_condition{node::value_node(5.0)}},
      },
      std::logical_and{}, true);
  std::cout << std::boolalpha << f5 << std::endl;
}

void store_test() {
  store store{node::empty_object()};
  std::cout << store << std::endl;

  store.write({
      {immut_list{"arango"s, "Plan"s, "Database"s, "myDB"s},
       set_operator{node::from_buffer_ptr(R"=({"name":"myDB", "replFact":2, "isBuilding":true})="_vpack)}},
      {immut_list{"arango"s, "Plan"s, "Version"s}, set_operator{node::value_node(1.0)}},
  });
  std::cout << store << std::endl;

  store.transact(
      {
          {{"arango"s, "Plan"s, "Database"s, "myDB"s, "isBuilding"s},
           equal_condition{node::value_node(true)}},
      },
      {
          {{"arango"s, "Plan"s, "Database"s, "myDB"s}, remove_operator{}},
          {{"arango"s, "Plan"s, "Version"s}, increment_operator{}},
      });
  std::cout << store << std::endl;
}

int main(int argc, char* argv[]) {
  // node_test();
  store_test();
}

#include <iostream>
#include <map>
#include <memory>

#include "helper-immut.h"

#include "node-operations.h"
#include "node.h"

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

int main(int argc, char* argv[]) {
  auto n = node::from_buffer_ptr(R"=({"key":{"hello":"world", "foo":12}})="_vpack);

  std::cout << *n << std::endl;

  auto m = node::from_buffer_ptr(R"=({"key":{"bar":12}, "foo":["blub"]})="_vpack);
  std::cout << *m << std::endl;

  std::cout << *n->overlay(m) << std::endl;

  auto p = n->set(immut_list{"key"s, "world"s}, node::value_node(12.0));

  std::cout << *p << std::endl;

  auto n1 = n->modify({
      {immut_list{"key"s, "hello"s},
       [](std::shared_ptr<const node> const&) { return node::value_node(7.0); }},
      {immut_list{"key"s, "foo"s},
       [](std::shared_ptr<const node> const&) { return node::value_node("42"s); }},
      {immut_list{"key"s, "foox"s, "bar"s},
       [](std::shared_ptr<const node> const&) { return node::value_node(false); }},
  });

  std::cout << *n << std::endl;
  std::cout << *n1 << std::endl;

  auto m1 = m->modify({
      {immut_list{"foo"s, "0"s, "bar"s},
       [](std::shared_ptr<const node> const&) { return node::value_node(false); }},
  });
  std::cout << "m1 " << *m1 << std::endl;
  auto m2 = m->modify({
      {immut_list{"foo"s, "x"s, "bar"s},
       [](std::shared_ptr<const node> const&) { return node::value_node(false); }},
  });
  std::cout << "m2 " << *m2 << std::endl;
  auto m3 = m->modify({
      {immut_list{"foo"s, "3"s, "bar"s},
       [](std::shared_ptr<const node> const&) { return node::value_node(false); }},
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

  auto r1 = q->modify({
      {immut_list{"baz"s}, pop_operator{}},
  });
  std::cout << *r1 << std::endl;
}

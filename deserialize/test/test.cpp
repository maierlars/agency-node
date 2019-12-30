#include <map>

#define DESERIALIZER_NO_VPACK_TYPES
#define DESERIALIZER_SET_TEST_TYPES

#include "test-types.h"

#include "deserializer.h"

#include "velocypack/Buffer.h"
#include "velocypack/Builder.h"
#include "velocypack/Iterator.h"
#include "velocypack/Parser.h"
#include "velocypack/Slice.h"

using namespace arangodb::velocypack;

using VPackBufferPtr = std::shared_ptr<Buffer<uint8_t>>;

static inline Buffer<uint8_t> vpackFromJsonString(char const* c) {
  Options options;
  options.checkAttributeUniqueness = true;
  Parser parser(&options);
  parser.parse(c);

  std::shared_ptr<Builder> builder = parser.steal();
  std::shared_ptr<Buffer<uint8_t>> buffer = builder->steal();
  Buffer<uint8_t> b = std::move(*buffer);
  return b;
}

static inline Buffer<uint8_t> operator"" _vpack(const char* json, size_t) {
  return vpackFromJsonString(json);
}

template <typename T>
using my_vector = std::vector<T>;
template <typename K, typename V>
using my_map = std::unordered_map<K, V>;

void test01() {
  auto buffer = R"=(["hello", true, 123.4])="_vpack;
  auto slice = deserializer::test::recording_slice::from_buffer(buffer);

  using deserial =
      deserializer::tuple_deserializer<deserializer::values::value_deserializer<std::string>,
                                       deserializer::values::value_deserializer<bool>,
                                       deserializer::values::value_deserializer<double>>;

  auto result = deserializer::deserialize_with<deserial>(slice);
  static_assert(
      std::is_same_v<decltype(result)::value_type, std::tuple<std::string, bool, double>>);
  std::cout << *slice.tape << std::endl;
}

void test02() {
  auto buffer = R"=([{"op":"bar"}, {"op":"foo"}])="_vpack;
  auto slice = deserializer::test::recording_slice::from_buffer(buffer);

  constexpr static const char op_name[] = "op";
  constexpr static const char bar_name[] = "bar";
  constexpr static const char foo_name[] = "foo";

  using op_deserial =
      deserializer::attribute_deserializer<op_name, deserializer::values::value_deserializer<std::string>>;

  using prec_deserial_pair =
      deserializer::value_deserializer_pair<deserializer::values::string_value<bar_name>, op_deserial>;
  using prec_deserial_pair_foo =
      deserializer::value_deserializer_pair<deserializer::values::string_value<foo_name>, op_deserial>;

  using deserial =
      deserializer::array_deserializer<deserializer::field_value_dependent_deserializer<op_name, prec_deserial_pair, prec_deserial_pair_foo>, my_vector>;

  auto result = deserializer::deserialize_with<deserial>(slice);

  static_assert(
      std::is_same_v<decltype(result)::value_type, std::vector<std::variant<std::string, std::string>>>);
  if (!result) {
    std::cerr << result.error().as_string() << std::endl;
  }
  std::cout << *slice.tape << std::endl;
}

void test03() {
  struct deserialized_type {
    my_map<std::string, std::variant<std::unique_ptr<deserialized_type>, std::string>> value;
  };

  struct recursive_deserializer {
    using plan = deserializer::map_deserializer<
        deserializer::conditional_deserializer<
            deserializer::condition_deserializer_pair<deserializer::is_object_condition, deserializer::unpack_proxy<recursive_deserializer, deserialized_type>>,
            deserializer::conditional_default<deserializer::values::value_deserializer<std::string>>>,
        my_map>;
    using factory = deserializer::utilities::constructor_factory<deserialized_type>;
    using constructed_type = deserialized_type;
  };

  auto buffer = R"=({"a":"b", "c":{"d":{"e":"false"}}})="_vpack;
  auto slice = deserializer::test::recording_slice::from_buffer(buffer);

  auto result = deserializer::deserialize_with<recursive_deserializer>(slice);

  if (!result) {
    std::cerr << result.error().as_string() << std::endl;
  }
  std::cout << *slice.tape << std::endl;
}

void test04() {
  struct non_default_constructible_type {
    explicit non_default_constructible_type(double){};
  };

  struct non_copyable_type {
    explicit non_copyable_type(double) {}
    non_copyable_type(non_copyable_type const&) = delete;
    non_copyable_type(non_copyable_type&&) noexcept = default;
  };

  using deserial = deserializer::tuple_deserializer<
      deserializer::utilities::constructing_deserializer<non_default_constructible_type, deserializer::values::value_deserializer<double>>,
      deserializer::utilities::constructing_deserializer<non_copyable_type, deserializer::values::value_deserializer<double>>>;

  auto buffer = R"=([12])="_vpack;
  auto slice = deserializer::test::recording_slice::from_buffer(buffer);

  auto result = deserializer::deserialize_with<deserial>(slice);

  if (!result) {
    std::cerr << result.error().as_string() << std::endl;
  }
  std::cout << *slice.tape << std::endl;
}

int main(int argc, char* argv[]) {
  test01();
  test02();
  test03();
}



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

void test01() {
  auto buffer = R"=(["hello", true, 123.4])="_vpack;
  auto slice = deserializer::test::recording_slice::from_buffer(buffer);

  using deserial =
      deserializer::tuple_deserializer<deserializer::values::value_deserializer<std::string>,
                                       deserializer::values::value_deserializer<bool>,
                                       deserializer::values::value_deserializer<double>>;

  auto result = deserializer::deserialize_with<deserial>(slice);
  std::cout << *slice.tape << std::endl;
}

template <typename T>
using my_vector = std::vector<T>;

void test02() {
  auto buffer = R"=([{"op":"bar"}, {"op":"foo"}])="_vpack;
  auto slice = deserializer::test::recording_slice::from_buffer(buffer);

  constexpr static const char op_name[] = "op";
  constexpr static const char bar_name[] = "bar";
  constexpr static const char foo_name[] = "foo";

  using deserial = deserializer::arrays::array_deserializer<
      deserializer::field_value_dependent::field_value_dependent_deserializer<
          op_name,
          deserializer::field_value_dependent::value_deserializer_pair<
              deserializer::values::string_value<bar_name>, deserializer::attribute_deserializer<op_name, deserializer::values::value_deserializer<std::string>>>,
          deserializer::field_value_dependent::value_deserializer_pair<
              deserializer::values::string_value<foo_name>, deserializer::attribute_deserializer<op_name, deserializer::values::value_deserializer<std::string>>>>,
      std::vector>;

  auto result = deserializer::deserialize_with<deserial>(slice);
  if (!result) {
    std::cerr << result.error().what() << std::endl;
  }
  std::cout << *slice.tape << std::endl;
}

int main(int argc, char* argv[]) {
  test01();
  test02();
}

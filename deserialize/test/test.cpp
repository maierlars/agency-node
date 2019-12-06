
#define DESERIALIZER_SET_TEST_TYPES
#include "test-types.h"

#define DESERIALIZER_NO_VPACK_TYPES
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

int main(int argc, char* argv[]) {

  auto buffer = R"=(["hello", true, 123.4])="_vpack;
  auto slice = deserializer::test::recording_slice::from_buffer(buffer);

  using deserial = deserializer::tuple_deserializer<deserializer::values::value_deserializer<std::string>,
      deserializer::values::value_deserializer<bool>, deserializer::values::value_deserializer<double>>;

  auto result = deserializer::deserialize_with<deserial>(slice);
  std::cout << *slice.tape << std::endl;

}

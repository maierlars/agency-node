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

template <typename T>
using my_vector = std::vector<T>;

void test02() {
  auto buffer = R"=([{"op":"bar"}, {"op":"fooz"}])="_vpack;
  auto slice = deserializer::test::recording_slice::from_buffer(buffer);

  constexpr static const char op_name[] = "op";
  constexpr static const char bar_name[] = "bar";
  constexpr static const char foo_name[] = "foo";

  using op_deserial =
      deserializer::attribute_deserializer<op_name, deserializer::values::value_deserializer<std::string>>;

  using prec_deserial_pair =
      deserializer::value_deserializer_pair<deserializer::values::string_value<bar_name>, op_deserial>;

  using deserial =
      deserializer::array_deserializer<deserializer::field_value_dependent_deserializer<op_name, prec_deserial_pair, prec_deserial_pair>,
                                       std::vector>;

  auto result = deserializer::deserialize_with<deserial>(slice);

  static_assert(
      std::is_same_v<decltype(result)::value_type, std::vector<std::variant<std::string, std::string>>>);
  if (!result) {
    std::cerr << result.error().as_string() << std::endl;
  }
  std::cout << *slice.tape << std::endl;
}

template <typename P>
struct make_unique_factory {
  using constructed_type = std::unique_ptr<P>;

  template <typename... S>
  auto operator()(S&&... s) -> constructed_type {
    return std::make_unique<P>(std::forward<S>(s)...);
  }
};

template <typename D, typename P = D>
struct unpack_proxy {
  using constructed_type = std::unique_ptr<P>;
  using plan = unpack_proxy<D, P>;
  using factory = make_unique_factory<P>;
};

template<typename D, typename P>
struct deserializer::executor::plan_result_tuple<unpack_proxy<D, P>> {
  using type = std::tuple<P>;
};

template <typename D, typename P, typename H>
struct deserializer::executor::deserialize_plan_executor<unpack_proxy<D, P>, H> {
  using proxy_type = typename D::constructed_type;
  using tuple_type = std::tuple<proxy_type>;
  using result_type = result<tuple_type, deserialize_error>;

  static auto unpack(::deserializer::slice_type s, typename H::state_type h) -> result_type {
    return deserialize_with<D, H>(s, h).map([](typename D::constructed_type&& v) {
      return std::make_tuple(std::move(v));
    });
  }
};

template <typename K, typename V>
using my_map = std::unordered_map<K, V>;

void test03() {
  struct deserialized_type {
    my_map<std::string, std::variant<std::unique_ptr<deserialized_type>, std::string>> value;
  };

  struct recursive_deserializer {
    using plan = deserializer::map_deserializer<
        deserializer::conditional_deserializer<
            deserializer::condition_deserializer_pair<deserializer::is_object_condition, unpack_proxy<recursive_deserializer, deserialized_type>>,
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

int main(int argc, char* argv[]) {
  test01();
  test02();
  test03();
}

#include "test-helper.h"

#include "deserialize/deserializer.h"

#include <iomanip>
#include <iostream>
#include <memory>

node_ptr node_from_file(std::string const& filename) {
  std::string str;

  auto const readTime = timed([&] {
    std::stringstream ss;
    std::ifstream fs;
    fs.open(filename);
    ss << fs.rdbuf();
    str = ss.str();
  });
  auto data = std::shared_ptr<Builder>{};

  auto const parseJson = timed([&] { data = Parser::fromJson(str); });

  auto nodePtr = node_ptr{};
  auto const parseSlice =
      timed([&] { nodePtr = node::from_slice(data->slice()); });

  auto const readTime_s =
      std::chrono::duration_cast<std::chrono::duration<double>>(readTime).count();
  auto const jsonTime_s =
      std::chrono::duration_cast<std::chrono::duration<double>>(parseJson).count();
  auto const sliceTime_s =
      std::chrono::duration_cast<std::chrono::duration<double>>(parseSlice).count();

  std::cout << "Read file: " << readTime_s << "s, "
            << "parse json: " << jsonTime_s << "s, "
            << "parse slice: " << sliceTime_s << "s"
            << "\n";

  return nodePtr;
}

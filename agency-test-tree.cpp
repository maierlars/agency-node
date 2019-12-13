#include <tuple>
#include <variant>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>

#include "velocypack/Buffer.h"
#include "velocypack/Builder.h"
#include "velocypack/Iterator.h"
#include "velocypack/Parser.h"
#include "velocypack/Slice.h"

using namespace arangodb::velocypack;

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

template <class Container>
void split(const std::string& str, Container& cont,
              char delim = '/')
{
    std::size_t current, previous = 0;
    current = str.find(delim);
    while (current != std::string::npos) {
        cont.push_back(str.substr(previous, current - previous));
        previous = current + 1;
        current = str.find(delim, previous);
    }
    cont.push_back(str.substr(previous, current - previous));
}

struct empty_object {};

std::ostream& operator<<(std::ostream &os, empty_object) {
  os << "(empty object)";
  return os;
}

class store {
  class node;

  using node_ptr = std::unique_ptr<node>;

  using value_type = std::variant<double, std::string, bool, empty_object, node_ptr>;
  using key_type = std::string;

  class node {
    using map_type = std::unordered_map<key_type, child>;

    friend store;
  };

  using map_type = std::map<key_type, value_type>;
  using map_iterator = map_type::iterator;

public:
  void set(key_type key, Slice const& slice) {
  }

  template<typename T>
  void get(key_type key, Buffer<T> &buffer) {
  }

  friend std::ostream& operator<<(std::ostream &os, store const& st);
};

std::ostream& operator<<(std::ostream &os, store const& st) {

  return os;
}

Buffer<uint8_t> operator ""_json(const char* src, size_t) {
  Buffer<uint8_t> buffer;
  {
    Builder builder(buffer);
    Parser parser(builder);
    parser.parse(src);
  }
  return std::move(buffer);
}



int main(int argc, char *argv[]) {

  std::fstream fs(argv[1]);
  std::stringstream ss;
  ss << fs.rdbuf(); //read the file
  std::string str = ss.str();

  auto data = Parser::fromJson(str);

  store s;

  using namespace std::chrono;

  auto startOfImport = std::chrono::steady_clock::now();
  s.set("import", data->slice());
  //std::cout << s;
  auto endOfImport = std::chrono::steady_clock::now();

  Buffer<uint8_t> res;

  auto startOfExport = std::chrono::steady_clock::now();
  s.get("import/arango/Supervision/Health", res);
  auto endOfExport = std::chrono::steady_clock::now();

  std::cout << Slice(res.data()).toJson() << std::endl;

  std::cout << "Import time " << (endOfImport - startOfImport).count() << std::endl;
  std::cout << "Export time " << (endOfExport - startOfExport).count() << std::endl;
}



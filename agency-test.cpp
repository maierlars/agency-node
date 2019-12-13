#include <tuple>
#include <variant>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <utility>

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
  using leaf_type = std::variant<double, std::string, bool, empty_object>;
  using key_type = std::string;

  using map_type = std::map<key_type, leaf_type>;
  using map_iterator = map_type::iterator;

public:
  void set(key_type key, Slice const& slice) {
    deleteRange(key);
    insertLeaves(key, slice);
  }

  template<typename T>
  void get(key_type key, Buffer<T> &buffer) {
    auto [first, last] = getKeyRange(key);
    Builder builder(buffer);
    buildObject(builder, first, last);
  }

  friend std::ostream& operator<<(std::ostream &os, store const& st);

private:


  struct level_object {
    Builder* _builder;
    std::string _label;

    level_object(Builder builder, std::string label) : _builder(&builder), _label(std::move(label)) {
      _builder->add(Value(_label));
      _builder->openObject();
    }
    ~level_object() { if (!_label.empty()) { _builder->close(); }}
    bool operator==(std::string const& other) { return _label == other; }
  };

  void buildObject(Builder& builder, map_iterator first, map_iterator last) {



    ObjectBuilder ob(&builder);
    std::vector<level_object> levels;

    for (auto entry = first; entry != last; entry++) {

      std::string const& key = entry->first;

      // split string
      std::vector<std::string> key_levels;
      split (key, key_levels);

      // find last common parent
      auto [mismatch, mismatch_key] = std::mismatch(levels.begin(), levels.end(), key_levels.begin());

      // close objects as required
      levels.erase(mismatch, levels.end());

      // open objects as required
      for (; mismatch_key != key_levels.end()-1; mismatch_key++) {
        levels.emplace_back(builder, std::move(*mismatch_key));
      }

      // store the value
      builder.add(Value(*mismatch_key));
      std::visit(overloaded {
        [&](empty_object const& v) { builder.add(Slice::emptyObjectSlice()); },
        [&](auto const& v) { builder.add(Value(v)); }
      }, entry->second);
    }
  }


  void deleteRange(key_type const& key) {
    auto [first, last] = getKeyRange(key);
    _map.erase(first, last);
  }

  void insertLeaves(key_type key, Slice const& slice) {
    if (slice.isNumber()) {
      _map[key].emplace<double>(slice.getNumber<double>());
    } else if (slice.isString()) {
      _map[key].emplace<std::string>(std::move(slice.copyString()));
    } else if (slice.isBool()) {
      _map[key].emplace<bool>(slice.getBool());
    } else if (slice.isEmptyObject()) {
      _map[key].emplace<empty_object>();
    } else if (slice.isObject()) {
      for (auto const& level : ObjectIterator(slice)) {
        insertLeaves(key + '/' + level.key.copyString(), level.value);
      }
    } else {
      //std::cerr << "dropping " << key << std::endl;
    }
  }

  std::pair<map_iterator, map_iterator> getKeyRange(key_type const& key) {
    if (key.empty()) {
      return std::make_pair(_map.begin(), _map.end());
    }
    key_type next_key = key;
    next_key.back() += 1;
    return std::make_pair(_map.lower_bound(key), _map.upper_bound(next_key));
  }

private:
  map_type _map;
};

std::ostream& operator<<(std::ostream &os, store const& st) {
  for (auto const& entry : st._map) {
    os << entry.first << ":";
    std::visit([&](auto const& v){ os << v; }, entry.second);
    os << std::endl;
  }

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



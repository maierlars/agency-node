#include <iomanip>
#include <iostream>

#include "helper-immut.h"

#include "node-conditions.h"
#include "store.h"
#include "test-helper.h"

#include "deserialize/deserializer.h"

using namespace std::string_literals;

void store_mem_test(std::string const& filename) {
  auto p = node_from_file(filename);

  auto path =
      immut_list<std::string>{"arango", "Plan", "Collections", "_system", "abc"};
  auto q = node::from_buffer_ptr(R"=({"name":"myDB", "replFact":2, "isBuilding":true})="_vpack);

  auto const n = 1'000'000;
  auto const dur = timed([&] {
    for (int i = 0; i < n; i++) {
      p = p->set(path, q);
    }
  });

  std::cout
      << "Time " << std::setprecision(3)
      << (double)std::chrono::duration_cast<std::chrono::duration<double>>(dur).count()
      << "s" << std::endl;
  std::cout
      << "avg  " << std::setprecision(3)
      << (double)std::chrono::duration_cast<std::chrono::microseconds>(dur).count() / n
      << "us" << std::endl;
}

int main(int argc, char* argv[]) {
  if (argc > 1) {
    store_mem_test(argv[1]);
    return EXIT_SUCCESS;
  } else {
    return EXIT_FAILURE;
  }
}

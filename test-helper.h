
#ifndef AGENCY_NODE_TEST_HELPER_H
#define AGENCY_NODE_TEST_HELPER_H

#include "node.h"

#include <string>

node_ptr node_from_file(std::string const& filename);

template <typename F>
auto timed(F&& lambda) {
  auto const start = std::chrono::steady_clock::now();
  lambda();
  auto const end = std::chrono::steady_clock::now();

  return end - start;
}

#endif  // AGENCY_NODE_TEST_HELPER_H

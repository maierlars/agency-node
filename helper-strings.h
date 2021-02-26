//
// Created by lars on 28.11.19.
//

#ifndef AGENCY_HELPER_STRINGS_H
#define AGENCY_HELPER_STRINGS_H
#include <optional>
#include <string>

template <typename T>
std::optional<T> string_to_number(std::string const& str) {
  T a = T();
  for (char c : str) {
    if ('0' <= c && c <= '9') {
      a = 10 * a + c - '0';
    } else {
      return {};
    }
  }
  return a;
}

#endif  // AGENCY_HELPER_STRINGS_H

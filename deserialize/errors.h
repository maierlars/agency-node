//
// Created by lars on 2019-12-04.
//

#ifndef VELOCYPACK_ERRORS_H
#define VELOCYPACK_ERRORS_H
#include <variant>
#include "gadgets.h"

struct error {
  using access_type = std::variant<std::string, std::size_t>;

  error() noexcept = default;
  explicit error(std::string message)
      : backtrace(), message(std::move(message)) {}
  std::vector<access_type> backtrace;
  std::string message;

  error& trace(std::string field) {
    backtrace.emplace_back(std::move(field));
    return *this;
  }
  error& trace(std::size_t index) {
    backtrace.emplace_back(index);
    return *this;
  };

  error& wrap(const std::string& wrap) {
    message = wrap + ':' + message;
    return *this;
  }

  operator std::string() const { return as_string(); }
  [[nodiscard]] std::string as_string() const {
    std::string result;
    for (auto i = backtrace.crbegin(); i != backtrace.crend(); i++) {
      std::visit(::deserializer::detail::gadgets::visitor{[&](std::string const& field) {
                                                            result += '.';
                                                            result += field;
                                                          },
                                                          [&](std::size_t index) {
                                                            result += '[';
                                                            result += std::to_string(index);
                                                            result += ']';
                                                          }},
                 *i);
    }

    result += ": ";
    result += message;
    return result;
  }
};

using deserialize_error = error;
/*
struct deserialize_error {
  explicit deserialize_error() noexcept = default;
  explicit deserialize_error(std::string msg) : msg(std::move(msg)) {}

  [[nodiscard]] std::string const& what() const { return msg; }
  [[nodiscard]] deserialize_error wrap(std::string const& wrap) const {
    return deserialize_error(wrap + ": " + msg);
  }

 private:
  std::string msg;
};*/

#endif  // VELOCYPACK_ERRORS_H

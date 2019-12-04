//
// Created by lars on 2019-12-04.
//

#ifndef VELOCYPACK_ERRORS_H
#define VELOCYPACK_ERRORS_H

struct deserialize_error {
  explicit deserialize_error() noexcept = default;
  explicit deserialize_error(std::string msg) : msg(std::move(msg)) {}

  [[nodiscard]] std::string const& what() const { return msg; }
  [[nodiscard]] deserialize_error wrap(std::string const& wrap) const {
    return deserialize_error(wrap + ": " + msg);
  }

 private:
  std::string msg;
};

#endif  // VELOCYPACK_ERRORS_H

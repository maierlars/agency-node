//
// Created by lars on 29.11.19.
//

#ifndef AGENCY_STORE_H
#define AGENCY_STORE_H
#include <mutex>
#include <shared_mutex>
#include "node.h"

struct store {

  store() = default;
  explicit store(node_ptr root) : root(std::move(root)) {}

  store(store const&) = delete;
  store& operator=(store const&) = delete;
  store(store &&) noexcept = delete;
  store& operator=(store &&) noexcept = delete;


  void write(std::vector<node::modify_action> const& operations) {
    std::unique_lock modify_guard(root_modify_mutex);
    set_internal(root->modify(operations));
  }

  void set(node_ptr new_root) {
    std::unique_lock modify_guard(root_modify_mutex);
    set_internal(std::move(new_root));
  }

  [[nodiscard]] node_ptr read() const noexcept {
    std::shared_lock guard(root_mutex);
    return root;
  }

 private:
  void set_internal(node_ptr new_root) {
    std::unique_lock guard(root_mutex);
    root = std::move(new_root);
  }


  std::mutex root_modify_mutex;
  std::shared_mutex root_mutex;

  node_ptr root;
};


#endif  // AGENCY_STORE_H

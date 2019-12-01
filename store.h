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
  store(store&&) noexcept = delete;
  store& operator=(store&&) noexcept = delete;

  node_ptr transact(std::vector<node::fold_action<bool>> const& preconditions,
                    std::vector<node::modify_action> const& operations) {
    std::unique_lock modify_guard(root_modify_mutex);
    bool preconditionsOk = root->fold<bool>(preconditions, std::logical_and{}, true);
    if (preconditionsOk) {
      return set_internal(root->modify(operations));
    }
    return nullptr;
  }

  bool check(std::vector<node::fold_action<bool>> const& conditions) {
    return read()->fold<bool>(conditions, std::logical_and{}, true);
  }

  node_ptr write(std::vector<node::modify_action> const& operations) {
    std::unique_lock modify_guard(root_modify_mutex);
    return set_internal(root->modify(operations));
  }

  [[deprecated]] node_ptr set(node_ptr new_root) {
    std::unique_lock modify_guard(root_modify_mutex);
    return set_internal(std::move(new_root));
  }

  [[nodiscard]] node_ptr read() const {
    std::shared_lock guard(root_mutex);
    return root;
  }

 private:
  node_ptr set_internal(node_ptr new_root) {
    // TODO assert that this thread holds root_modify_mutex
    std::unique_lock guard(root_mutex);
    return root = std::move(new_root);
  }

  mutable std::mutex root_modify_mutex;
  mutable std::shared_mutex root_mutex;

  node_ptr root;
};

std::ostream& operator<<(std::ostream& ostream, store const& store) {
  return ostream << *store.read();
}

#endif  // AGENCY_STORE_H

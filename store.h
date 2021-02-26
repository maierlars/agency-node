//
// Created by lars on 29.11.19.
//

#ifndef AGENCY_STORE_H
#define AGENCY_STORE_H

#include "node-operations.h"
#include "node.h"

#include <atomic>
#include <mutex>
#include <queue>
#include <shared_mutex>

template <typename T>
struct store_ttl {
  using clock_type = std::chrono::system_clock;
  using store_type = T;

  store_ttl() noexcept = default;
  store_ttl(store_ttl const&) = delete;
  store_ttl& operator=(store_ttl const&) = delete;
  store_ttl(store_ttl&&) noexcept = delete;
  store_ttl& operator=(store_ttl&&) noexcept = delete;

  // path *must* be normalized!
  void set_ttl(std::string_view path, clock_type::duration ttl) {
    std::unique_lock guard(ttl_queue_guard);

    auto at = std::find(ttl_list.begin(), ttl_list.end(),
                        [&path](ttl_entry const& e) { return e.path == path; });

    clock_type::time_point end_of_life = clock_type::now() + ttl;

    if (at == ttl_list.end()) {
      ttl_list.emplace_back(path, end_of_life);
    } else {
      at->end_of_life = end_of_life;
    }
  }

  void remove_ttl(std::string_view path) {
    std::unique_lock guard(ttl_queue_guard);
    ttl_list.erase(std::remove(ttl_list.begin(), ttl_list.end(), path), ttl_list.end());
  }

 public:
  void stop() {
    std::unique_lock guard(ttl_queue_guard);
    stopped.store(true, std::memory_order_release);
    wait_for_stopped.notify_all();
  }

  void run_queue_thread() {
    while (!stopped.load(std::memory_order_acquire)) {
      std::unique_lock guard(ttl_queue_guard);

      auto now = clock_type::now();

      std::vector<node::transform_action> remove_actions;

      auto const isExpired = [this, &now, &remove_actions](ttl_entry const& e) {
        return e.end_of_life > now;
      };

      for (auto const& it : ttl_list) {
        if (isExpired(it)) {
          remove_actions.emplace_back(std::move(it.path), remove_operator{});
        }
      }

      ttl_list.erase(std::remove_if(ttl_list.begin(), ttl_list.end(), isExpired),
                     ttl_list.end());

      if (!remove_actions.empty()) {
        self().modification(remove_actions);
      }

      using namespace std::chrono_literals;
      wait_for_stopped.wait_for(guard, 1s);
    }
  }

 private:
  std::atomic<bool> stopped = false;
  std::condition_variable wait_for_stopped;

  struct ttl_entry {
    std::string path;
    clock_type::time_point end_of_life;

    explicit ttl_entry(std::string path, clock_type::time_point end_of_life)
        : path(std::move(path)), end_of_life(end_of_life) {}
  };

  std::vector<ttl_entry> ttl_list;
  mutable std::mutex ttl_queue_guard;

  T& self() { return *dynamic_cast<T*>(this); }
};

struct store_base : public store_ttl<store_base> {
  store_base() = default;
  explicit store_base(node_ptr root) : root(std::move(root)) {}

  store_base(store_base const&) = delete;
  store_base& operator=(store_base const&) = delete;
  store_base(store_base&&) noexcept = delete;
  store_base& operator=(store_base&&) noexcept = delete;

  node_ptr transact(std::vector<node::fold_action<bool>> const& preconditions,
                    std::vector<node::transform_action> const& operations) {
    std::unique_lock modify_guard(root_modify_mutex);
    // TODO the root_modify_mutex can be unlocked as soon as the first precondition fails
    bool preconditionsOk = root->fold<bool>(preconditions, std::logical_and{}, true);
    if (preconditionsOk) {
      return set_internal(root->transform(operations));
    }
    return nullptr;
  }

  bool check(std::vector<node::fold_action<bool>> const& conditions) {
    return read()->fold<bool>(conditions, std::logical_and{}, true);
  }

  node_ptr write(std::vector<node::transform_action> const& operations) {
    std::unique_lock modify_guard(root_modify_mutex);
    return set_internal(root->transform(operations));
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

std::ostream& operator<<(std::ostream& ostream, store_base const& store) {
  return ostream << *store.read();
}

#endif  // AGENCY_STORE_H

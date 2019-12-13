#ifndef AGENCY_NODE_FUTURES_H
#define AGENCY_NODE_FUTURES_H

#include <utility>
#include <optional>
#include <functional>
#include <atomic>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <cassert>

#include "deserialize/types.h"

struct scheduler {
  static void queue(std::function<void(void)> const& handler) noexcept {
    handler();
  }
};

template<typename T>
struct promise;
template<typename T>
struct future;

template<typename T>
struct shared_state {

  enum class state {
    EMPTY,
    CALLBACK,
    VALUE,
    DONE
  };

  shared_state(shared_state&&) = delete;
  shared_state(shared_state const&) = delete;
  shared_state& operator=(shared_state&&) = delete;
  shared_state& operator=(shared_state const&) = delete;

  template<typename S = T>
  explicit shared_state(S&& s) : _state(state::VALUE), _callback(), _value(std::forward<S>(s)) {}
  shared_state() noexcept = default;

  std::atomic<state> _state;
  std::function<void(T&&)> _callback;
  std::optional<T> _value;

  template<typename S = T>
  void setValue(S && s) noexcept {
    _value = std::forward<S>(s); transit<state::VALUE, state::CALLBACK>();
  }
  void setCallback(std::function<void(T&&)> cb) noexcept { _callback = cb; transit<state::CALLBACK, state::VALUE>(); };

 private:
  template<state A, state B>
  void transit() {
    state expected = state::EMPTY;
    if (_state.compare_exchange_strong(expected, A)) {
      return ;
    }

    assert(expected == B);
    if (_state.compare_exchange_strong(expected, state::DONE)) {
      _callback(std::move(*_value));
      return;
    }

    assert(false);
  }
};

template<typename T>
std::pair<future<T>, promise<T>> make_promise() noexcept;
template<typename T>
future<T> make_future(T&&) noexcept;

template<typename T>
struct promise {

  promise() = delete;
  promise(promise const&) = default;
  promise(promise&&) noexcept = default;
  promise& operator=(promise const&) = delete;
  promise& operator=(promise&&) noexcept = default;

  void set(T t) && noexcept {
    state->setValue(std::move(t));
    state.reset();
  };

  std::shared_ptr<shared_state<T>> state;

 private:
  explicit promise(std::shared_ptr<shared_state<T>> state) : state(std::move(state)) {}
  friend std::pair<future<T>, promise<T>> make_promise<T>() noexcept;
};

template<typename>
struct is_future : std::false_type {};
template<typename T>
struct is_future<future<T>> : std::true_type {};
template<typename T>
constexpr auto is_future_v = is_future<T>::value;
template<typename>
struct is_unit : std::false_type {};
template<>
struct is_unit<unit_type> : std::true_type {};
template<typename T>
constexpr auto is_unit_v = is_unit<T>::value;

template<typename T>
struct future {
  using return_type = T;

  future() = delete;
  future(future const&) = delete;
  future(future&&) noexcept = default;
  future& operator=(future const&) = delete;
  future& operator=(future&&) noexcept = default;

  std::shared_ptr<shared_state<T>> state;

  template<typename F, typename S = std::invoke_result_t<F, T&&>,
      std::enable_if_t<!is_future_v<S> && !std::is_void_v<S>, int> = 0>
  auto then(F&& f) && noexcept -> future<S> {
    auto [fp, p] = make_promise<S>();

    state->setCallback([p = std::move(p), f = std::forward<F>(f)](T && t) mutable {
      std::move(p).set(f(std::move(t)));
    });
    state.reset();

    return std::move(fp);
  }

  template<typename F, typename S = std::invoke_result_t<F, T&&>,
      std::enable_if_t<std::is_void_v<S>, int> = 0>
  auto then(F&& f) && noexcept -> future<unit_type> {
    auto [fp, p] = make_promise<unit_type>();

    state->setCallback([p = std::move(p), f = std::forward<F>(f)](T && t) mutable {
      f(std::move(t));
      std::move(p).set(unit_type{});
    });
    state.reset();

    return std::move(fp);
  }

  template<typename F, typename S = std::invoke_result_t<F, T&&>,
      std::enable_if_t<is_future_v<S>, int> = 0, typename R = typename S::return_type>
  auto then(F&& f) && noexcept -> future<R> {
    auto [fp, p] = make_promise<R>();

    state->setCallback([p = std::move(p), f = std::forward<F>(f)](T && t) mutable {
      f(std::move(t))
          .then([p = std::move(p)](R && r) mutable {
            std::move(p).set(std::move(r));
          });
    });
    state.reset();

    return std::move(fp);
  }

  // passes execution to the scheduler at this point of the stack trace
  auto reschedule() && noexcept -> future<T> {
    auto [f, p] = make_promise<T>();

    this->then([p = std::move(p)](T&& t) mutable {
      scheduler::queue([p = std::move(p), t = std::move(t)]() mutable {
        std::move(p).set(std::move(t));
      });
    });

    return f;
  }

 private:
  future(std::shared_ptr<shared_state<T>> state) : state(std::move(state)) {}
  friend future<T> make_future<T>(T&&) noexcept;
  friend std::pair<future<T>, promise<T>> make_promise<T>() noexcept;
};

template<typename T>
std::pair<future<T>, promise<T>> make_promise() noexcept {
  auto state = std::make_shared<shared_state<T>>();
  return std::make_pair(future{state}, promise{state});
}

template<typename T>
future<T> make_future(T&& t) noexcept {
  auto state = std::make_shared<shared_state<T>>(std::forward<T>(t));
  return future<T>{state};
}


#endif  // AGENCY_NODE_FUTURES_H

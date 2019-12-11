#ifndef AGENCY_IMMUTABLE_LIST
#define AGENCY_IMMUTABLE_LIST


template<typename T>
struct immutable_list {

  immutable_list() noexcept = default;
  immutable_list(immutable_list const&) = default;
  immutable_list(immutable_list&&) noexcept = default;

  immutable_list(tail_container const& t) : _head(t._tail_start) {}

  immutable_list& operator=(immutable_list const&) = default;
  immutable_list& operator=(immutable_list&&) noexcept = default;

  explicit immutable_list(std::initializer_list<T> list) noexcept {
    container_pointer current;
    for (auto &element : list) {
      container_pointer next = std::make_shared<element_container>(nullptr, element);
      if (head == nullptr) {
        head = next;
      } else {
        current->next = next;
      }
      current = next;
    }
  };

  immutable_list<T> push_front(T t) const noexcept {
    return immutable_list(std::make_shared<element_container>(head, std::move(t)));
  }

  template<typename... As>
  immutable_list<T> emplace_front(As&&... a) const noexcept {
    return immutable_list(std::make_shared<element_container>(head, std::forward<As>(a)...));
  }

  operator bool() const noexcept { return head.operator bool(); }
  bool empty() const noexcept { return head; }
  immutable_list<T> const tail() const noexcept { return immutable_list(head ? head->next : nullptr); }
  immutable_list<T> tail() noexcept { return immutable_list(head ? head->next : nullptr); }
  T const& head() const noexcept { return head->value; }
  T& head() noexcept { return head->value; }

  bool operator==(immutable_list<T> const& other) const noexcept {
    if (head == nullptr || other.head == nullptr) {
      return head == other.head;
    }

    if (head->value == other.head->value) {
      return tail() == other.tail();
    }

    return false;
  }

  bool has_prefix(immutable_list<T> const& prefix) const noexcept {
    if (prefix.head == nullptr) {
      return true;
    }
    if (head == nullptr) {
      return false;
    }

    if (head->value == prefix.head->value) {
      return tail().has_prefix(prefix.tail());
    }

    return false;
  }

  bool operator<(immutable_list<T> const& other) const noexcept {
    if (other.head == nullptr) {
      return false;
    }
    if (head == nullptr) {
      return true;
    }

    if (head->value < other.head->value) {
      return tail() < other.tail();
    }
    return false;
  }

  template<int I>
  std::conditional_t<I == 0, T const&, immutable_list<T>> get() const noexcept {
    static_assert(0 <= I && I <= 1);
    if constexpr (I == 0) {
      return head();
    } else if constexpr (I == 1) {
      return tail();
    }
  }

 public:
  class tail_container {
    container_pointer const& _tail_start;

    tail_container(container_pointer const& tail) : _tail_start(tail) {}
    friend immutable_list;
  };

 private:
  struct element_container;
  using container_pointer = std::shared_ptr<element_container>;

  struct element_container {
    container_pointer next;
    T value;

    template<typename... As>
    explicit element_container(container_pointer next, As&&... a) : next(std::move(next)), value(std::forward<As>(a)...) {}
    explicit element_container(container_pointer next, T value) : next(std::move(next)), value(std::move(value)) {}
  };


  explicit immutable_list(container_pointer head) noexcept : head(std::move(head)) {}
  container_pointer head;
};

template<typename T>
struct std::tuple_size<immutable_list<T>> : std::integral_constant<std::size_t, 2> {};
template<typename T>
struct std::tuple_element<0, immutable_list<T>> { using type = T; };
template<typename T>
struct std::tuple_element<1, immutable_list<T>> { using type = immutable_list<T>; };


#endif  // AGENCY_IMMUTABLE_LIST

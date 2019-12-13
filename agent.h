#ifndef _AGENT_H
#define _AGENT_H

#include <deque>
#include <map>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "deserialize/types.h"
#include "futures.h"
#include "node.h"

#include "immer/flex_vector.hpp"

class check {
  // TODO
  // std::variant<> value;
};

class modification {
  // TODO
  // std::variant<> value;
};

class precondition {
  std::map<std::string, check> value;
};

class modification_operations {
  std::map<std::string, modification> value;
};

using path = std::string;

class conditional_modification {
  std::pair< precondition, modification_operations > value;
};

class combined_read {
  std::vector< path > value;
};

class envelope {
  std::vector<std::variant<conditional_modification, combined_read>> value;
};

class read_result {
  std::map<std::string, node_ptr> value;
};

using raft_id = uint64_t;
class write_result {
  std::optional<raft_id> value;
};

class transaction_error { /* TODO */ };

class trx_result {
  std::variant<read_result, write_result> value;
};

class envelope_result {
  std::vector< result<trx_result, transaction_error> > value;
};

struct log_entry {};

struct log_list {
  std::vector<log_entry> value;
};
struct log_error {};

using log_result = result<log_list, log_error>;

class transient_store { /* TODO */ };

class replicated_store {
  using id_node_pair = std::pair<raft_id , node_ptr>;
  using store_deque = std::deque<id_node_pair>;
  using log_deque = immer::flex_vector<log_entry>;


  store_deque state;
  log_deque log;


  [[nodiscard]] id_node_pair spearhead() const { return state.back(); }
  [[nodiscard]] id_node_pair readDB() const { return state.front(); }
};





struct snapshot {
  node_ptr store;
  // TODO ttl and stuff
};

struct data_store {

  struct persist_error {};
  struct load_error {};
  using persist_result = result<unit_type, persist_error>;
  template<typename R>
  using load_result = result<R, load_error>;

  [[nodiscard]] future<persist_result> persist_log(raft_id, envelope);
  [[nodiscard]] future<persist_result> persist_snapshot(snapshot);
  [[nodiscard]] future<persist_result> persist_election();


  [[nodiscard]] load_result<unit_type> load_snapshot();
};



class agent {
 public:
  // executes the envelope at spearhead and waits for it to be committed
  [[nodiscard]] future<envelope_result> transact(envelope);
  // executes the envelope on the volatile transient store
  [[nodiscard]] future<envelope_result> transient(envelope);

  // executes the envelope on the readDB. having a write in the envelope is a bad request.
  [[nodiscard]] envelope_result read(envelope) const;

  // returns a list of all committed log ids bigger than the given id
  // TODO is there a way to hand out a range using immutable ds ?
  [[nodiscard]] log_result read_log(raft_id) const;

 private:
  transient_store transient_store;
  replicated_store replicated_store;
};

#endif  //_AGENT_H

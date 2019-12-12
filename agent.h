////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2019 ArangoDB GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is ArangoDB GmbH, Cologne, Germany
///
/// @author Tobias Gödderz
////////////////////////////////////////////////////////////////////////////////

#ifndef _AGENT_H
#define _AGENT_H

#include <map>
#include <string>
#include <utility>
#include <variant>
#include <vector>
#include "operation-deserializer.h"

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

class transient_store { /* TODO */ };
class replicated_store { /* TODO */ };

class agent {
 public:

  envelope_result transact(envelope);

  envelope_result read(envelope);

  envelope_result transient(envelope);

 private:

  transient_store transientStore;
  replicated_store replicatedStore;
};

#endif  //_AGENT_H

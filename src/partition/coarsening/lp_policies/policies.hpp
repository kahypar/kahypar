#ifndef LP_POLICIES_HPP_
#define LP_POLICIES_HPP_ 1

#include "lib/definitions.h"
#include "partition/Configuration.h"

namespace lpa_hypergraph
{
  struct BasePolicy
  {
    virtual ~BasePolicy() {};

    static Configuration config_;
  };
};

using defs::NodeData;
using defs::EdgeData;
using defs::Label;
using defs::MyHashMap;


#include "check_finished_policies.hpp"
#include "collect_information_policies.hpp"
#include "compute_new_label_policies.hpp"
#include "compute_score_policies.hpp"
#include "gain_policies.hpp"
#include "node_initialization_policies.hpp"
#include "edge_initialization_policies.hpp"
#include "permutate_nodes_policies.hpp"
#include "permutate_sample_labels_policies.hpp"
#include "update_information_policies.hpp"

#endif

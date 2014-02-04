/***************************************************************************
 *  $(filename)
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/
#include <array>
#include <iostream>

#include "lib/GitRevision.h"
#include "lib/datastructure/Hypergraph.h"
#include "lib/datastructure/PriorityQueue.h"
#include "lib/definitions.h"
#include "lib/macros.h"
#include "partition/Configuration.h"
#include "partition/coarsening/HeuristicHeavyEdgeCoarsener.h"
#include "partition/coarsening/Rater.h"
#include "partition/coarsening/RatingTieBreakingPolicies.h"

int main(int, char**) {
  typedef datastructure::PriorityQueue<int, int, std::numeric_limits<int> > RefinementPQ;
  std::cout << "test:::" << std::endl;
  RefinementPQ* q1 = new RefinementPQ(4876);
  RefinementPQ* q2 = new RefinementPQ(4876);

  //    q1->insert(4900,2);
  //q2->insert(4900,5000);

// delete q1;
// delete q2;
}

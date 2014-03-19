/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_REFINEMENT_IREFINER_H_
#define SRC_PARTITION_REFINEMENT_IREFINER_H_
#include <string>


namespace partition {
template <class Hypergraph>
class IRefiner {
  private:
  typedef typename Hypergraph::HypernodeID HypernodeID;
  typedef typename Hypergraph::HyperedgeWeight HyperedgeWeight;

  public:
  virtual void refine(HypernodeID u, HypernodeID v, HyperedgeWeight& best_cut,
                      double max_imbalance, double& best_imbalance) = 0;
  virtual void initialize() { }
  virtual int numRepetitions() = 0;
  virtual std::string policyString() const = 0;
  virtual ~IRefiner() { }
};
} //namespace partition

#endif  // SRC_PARTITION_REFINEMENT_IREFINER_H_

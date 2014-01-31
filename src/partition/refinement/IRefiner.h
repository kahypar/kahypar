#ifndef PARTITION_REFINEMENT_IREFINER_H_
#define PARTITION_REFINEMENT_IREFINER_H_

namespace partition {
template <class Hypergraph>
class IRefiner {
  private:
  typedef typename Hypergraph::HypernodeID HypernodeID;
  typedef typename Hypergraph::HyperedgeWeight HyperedgeWeight;

  public:
  virtual void refine(HypernodeID u, HypernodeID v, HyperedgeWeight& best_cut,
                      double max_imbalance, double& best_imbalance) = 0;
  virtual ~IRefiner() { }
};
} //namespace partition

#endif  // PARTITION_REFINEMENT_IREFINER_H_

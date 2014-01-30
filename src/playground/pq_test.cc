#include "lib/datastructure/Hypergraph.h"
#include "lib/datastructure/PriorityQueue.h"
#include "lib/definitions.h"
#include "partition/Configuration.h"
#include "partition/coarsening/HeuristicHeavyEdgeCoarsener.h"
#include "partition/coarsening/Rater.h"
#include "partition/coarsening/RatingTieBreakingPolicies.h"
#include <array>
#include <iostream>

class A {
 public:
  virtual void print() = 0;
};

class BBase : public A {
 protected:
  int x = 5;

 public:
  void doStuff() {
    std::cout << "doStuff" << x << std::endl;
  }

};

class C : public BBase {
 public:
  void print() {
    std::cout << "C->print" << std::endl;
    BBase::doStuff();
  }

};

class D : public BBase {

  void print() {
    std::cout << "D->print" << std::endl;
    BBase::x = 7;
    BBase::doStuff();
  }

};

int main(int, char**) {
  typedef datastructure::PriorityQueue<int, int, std::numeric_limits<int> > RefinementPQ;
  std::cout << "test:::" << std::endl;
  RefinementPQ* q1 = new RefinementPQ(4876);
  RefinementPQ* q2 = new RefinementPQ(4876);

  //    q1->insert(4900,2);
  //q2->insert(4900,5000);

  delete q1;
  delete q2;
  std::cout << "::::test" << std::endl;

  A* ac = new C();
  A* ad = new D();

  datastructure::HyperedgeIndexVector index_vector;
  datastructure::HyperedgeVector edge_vector;

  ac->print();
  ad->print();
  datastructure::HypergraphType hgr(0, 0, index_vector, edge_vector);
  partition::Configuration<datastructure::HypergraphType> conf;
  typedef partition::Rater<datastructure::HypergraphType, defs::RatingType,
                           partition::RandomRatingWins> RandomWinsRater;
  partition::HeuristicHeavyEdgeCoarsener<datastructure::HypergraphType, RandomWinsRater> hec(hgr, conf);


}


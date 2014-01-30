#ifndef PARTITION_COARSENER_H_
#define PARTITION_COARSENER_H_

#include <stack>
#include <unordered_map>

#include "lib/datastructure/Hypergraph.h"
#include "lib/datastructure/PriorityQueue.h"
#include "partition/Configuration.h"
#include "partition/coarsening/Rater.h"
#include "partition/refinement/TwoWayFMRefiner.h"

#ifndef NSELF_VERIFICATION
#include "partition/Metrics.h"
#include "external/Utils.h"
#endif

namespace partition {
using datastructure::PriorityQueue;
using datastructure::MetaKeyDouble;

template <class Hypergraph, class Rater>
class Coarsener{
 private:
  typedef typename Hypergraph::HypernodeID HypernodeID;
  typedef typename Hypergraph::HyperedgeID HyperedgeID;
  typedef typename Hypergraph::HypernodeWeight HypernodeWeight;
  typedef typename Hypergraph::HyperedgeWeight HyperedgeWeight;
  typedef typename Hypergraph::ContractionMemento Memento;
  typedef typename Hypergraph::IncidenceIterator IncidenceIterator;
  typedef typename Hypergraph::HypernodeIterator HypernodeIterator;
  typedef typename Rater::Rating HeavyEdgeRating;
  typedef typename Rater::RatingType RatingType;
  typedef std::unordered_multimap<HypernodeID, HypernodeID> TargetToSourcesMap;
  
  struct CoarseningMemento {
    int one_pin_hes_begin;   // start of removed single pin hyperedges
    int one_pin_hes_size;    // # removed single pin hyperedges
    int parallel_hes_begin;  // start of removed parallel hyperedges
    int parallel_hes_size;   // # removed parallel hyperedges
    Memento contraction_memento;
    CoarseningMemento(Memento contraction_memento_) :
        one_pin_hes_begin(0),
        one_pin_hes_size(0),
        parallel_hes_begin(0),
        parallel_hes_size(0),
        contraction_memento(contraction_memento_) {}
  };

  struct Fingerprint {
    HyperedgeID id;
    HyperedgeID hash;
    HypernodeID size;
    Fingerprint(HyperedgeID id_, HyperedgeID hash_, HypernodeID size_) :
        id(id_),
        hash(hash_),
        size(size_) {}
  };

  struct ParallelHE {
    HyperedgeID representative_id;
    HyperedgeID removed_id;
    ParallelHE(HyperedgeID representative_id_, HyperedgeID removed_id_) :
        representative_id(representative_id_),
        removed_id(removed_id_) {}
  };
  
 public:  
  Coarsener(Hypergraph& hypergraph, const Configuration<Hypergraph>& config) :
      _hg(hypergraph),
      _config(config),
      _rater(_hg, _config),
      _history(),
      _removed_single_node_hyperedges(),
      _removed_parallel_hyperedges(),
      _fingerprints(),
      _contained_hypernodes(_hg.initialNumNodes()),
      _pq(_hg.initialNumNodes(), _hg.initialNumNodes()) {}
  
  void coarsen(int limit) {
    ASSERT(_pq.empty(), "coarsen() can only be called once");

    std::vector<HypernodeID> target(_hg.initialNumNodes());
    TargetToSourcesMap sources;
    
    rateAllHypernodes(target, sources);

    HypernodeID rep_node;
    HypernodeID contracted_node;
    HeavyEdgeRating rating;
    while (!_pq.empty() && _hg.numNodes() > limit) {
      rep_node = _pq.max();
      contracted_node = target[rep_node];
       // PRINT("Contracting: (" << rep_node << ","
       //       << target[rep_node] << ") prio: " << _pq.maxKey());

      ASSERT(_hg.nodeWeight(rep_node) + _hg.nodeWeight(target[rep_node])
             <= _rater.thresholdNodeWeight(),
             "Trying to contract nodes violating maximum node weight");

      performContraction(rep_node, contracted_node);
      _pq.remove(contracted_node);
      removeMappingEntryOfNode(contracted_node, target[contracted_node], sources);

      removeSingleNodeHyperedges(rep_node);
      removeParallelHyperedges(rep_node);

      rating = _rater.rate(rep_node);
      updatePQandMappings(rep_node, rating, target, sources);

      reRateHypernodesAffectedByContraction(rep_node, contracted_node, target, sources);
      reRateHypernodesAffectedByContraction(contracted_node, rep_node, target, sources);

      reRateHypernodesAffectedByParallelHyperedgeRemoval(target, sources);
    }
  }

  void uncoarsen(std::unique_ptr<Refiner<Hypergraph>>& refiner) {
    double current_imbalance = metrics::imbalance(_hg);
    HyperedgeWeight current_cut = metrics::hyperedgeCut(_hg);

    while(!_history.empty()) {  
#ifndef NDEBUG
      HyperedgeWeight old_cut = current_cut;
#endif
      
      restoreParallelHyperedges(_history.top());
      restoreSingleNodeHyperedges(_history.top());

      // PRINT("Uncontracting: (" << _history.top().contraction_memento.u << ","
      //       << _history.top().contraction_memento.v << ")");

      _hg.uncontract(_history.top().contraction_memento);
      refiner->refine(_history.top().contraction_memento.u, _history.top().contraction_memento.v,
                     current_cut, _config.partitioning.epsilon, current_imbalance);
      _history.pop();
      
      ASSERT(current_cut <= old_cut, "Cut increased during uncontraction");
    }
    ASSERT(current_imbalance <= _config.partitioning.epsilon,
           "balance_constraint is violated after uncontraction:" << current_imbalance
           << " > " << _config.partitioning.epsilon);
  }

 private:
  FRIEND_TEST(ACoarsener, SelectsNodePairToContractBasedOnHighestRating);
  
  void performContraction(HypernodeID rep_node, HypernodeID contracted_node) {
    _history.emplace(_hg.contract(rep_node, contracted_node));
  }

  void removeMappingEntryOfNode(HypernodeID hn, HypernodeID hn_target,
                                TargetToSourcesMap& sources) {
    auto range = sources.equal_range(hn_target);
    for (auto iter = range.first; iter != range.second; ++iter) {
      if (iter->second == hn) {
        //PRINT("********** removing node " << hn << " from entries with key " << target[hn]);
        sources.erase(iter);
        break;
      }
    }
  }

  void restoreSingleNodeHyperedges(const CoarseningMemento& memento) {
    for (int i = memento.one_pin_hes_begin + memento.one_pin_hes_size - 1;
         i >= memento.one_pin_hes_begin; --i) {
      ASSERT(i < _removed_single_node_hyperedges.size(), "Index out of bounds");
      // PRINT("*** restore single-node HE " << _removed_single_node_hyperedges[i]);
      _hg.restoreEdge(_removed_single_node_hyperedges[i]);
    }
  }

  void restoreParallelHyperedges(const CoarseningMemento& memento) {
    for (int i = memento.parallel_hes_begin + memento.parallel_hes_size - 1;
         i >= memento.parallel_hes_begin; --i) {
      ASSERT(i < _removed_parallel_hyperedges.size(), "Index out of bounds");
      // PRINT("*** restore HE " << _removed_parallel_hyperedges[i].removed_id
      //       << " which is parallel to " << _removed_parallel_hyperedges[i].representative_id);
      _hg.restoreEdge(_removed_parallel_hyperedges[i].removed_id);
      _hg.setEdgeWeight(_removed_parallel_hyperedges[i].representative_id,
                        _hg.edgeWeight(_removed_parallel_hyperedges[i].representative_id) -
                        _hg.edgeWeight(_removed_parallel_hyperedges[i].removed_id));
    }
  }

  void rateAllHypernodes(std::vector<HypernodeID>& target,
                         TargetToSourcesMap& sources) {
    HeavyEdgeRating rating;
    forall_hypernodes(hn, _hg) {
      rating = _rater.rate(*hn);
      if (rating.valid) {
        _pq.insert(*hn, rating.value);
        target[*hn] = rating.target;
        sources.insert({rating.target, *hn});
      }
    } endfor    
  }

  void reRateHypernodesAffectedByParallelHyperedgeRemoval(std::vector<HypernodeID>& target,
                                                          TargetToSourcesMap& sources) {
    HeavyEdgeRating rating;
    for (int i = _history.top().parallel_hes_begin; i != _history.top().parallel_hes_begin +
                 _history.top().parallel_hes_size; ++i) {
      forall_pins(pin, _removed_parallel_hyperedges[i].representative_id, _hg) {
          rating = _rater.rate(*pin);
          updatePQandMappings(*pin, rating, target, sources);
      } endfor
    }
  }

  void reRateHypernodesAffectedByContraction(HypernodeID hn, HypernodeID contraction_node,
                                             std::vector<HypernodeID>& target,
                                             TargetToSourcesMap& sources) {
    HeavyEdgeRating rating;
    auto source_range = sources.equal_range(hn);
    auto source_it = source_range.first;
    while (source_it != source_range.second) {
      if (source_it->second == contraction_node) {
        sources.erase(source_it++);
      } else {
        //PRINT("rerating HN " << source_it->second << " which had " << hn << " as target");
        rating = _rater.rate(source_it->second);
        // updatePQandMappings might invalidate source_it.
        HypernodeID source_hn = source_it->second;
        ++source_it;
        updatePQandMappings(source_hn, rating, target, sources);
      }
    }
  }

  void removeSingleNodeHyperedges(HypernodeID u) {
    ASSERT(_history.top().contraction_memento.u == u,
           "Current coarsening memento does not belong to hypernode" << u);
    _history.top().one_pin_hes_begin = _removed_single_node_hyperedges.size();
    IncidenceIterator begin, end;
    std::tie(begin, end) = _hg.incidentEdges(u);
    for (IncidenceIterator he_it = begin; he_it != end; ++he_it) {
      if (_hg.edgeSize(*he_it) == 1) {
        _removed_single_node_hyperedges.push_back(*he_it);
        ++_history.top().one_pin_hes_size;
        // PRINT("*** removing single-node HE " << *he_it);
        // _hg.printEdgeState(*he_it);
        _hg.removeEdge(*he_it);
        --he_it;
        --end;
      }
    }
  }

  void removeParallelHyperedges(HypernodeID u) {
    ASSERT(_history.top().contraction_memento.u == u,
           "Current coarsening memento does not belong to hypernode" << u);
    _history.top().parallel_hes_begin = _removed_parallel_hyperedges.size();

    createFingerprints(u);
    std::sort(_fingerprints.begin(), _fingerprints.end(),
              [](const Fingerprint& a, const Fingerprint& b) {return a.hash < b.hash;});

    for (size_t i = 0; i < _fingerprints.size(); ++i) {
      size_t j = i + 1;
      if (j < _fingerprints.size() && _fingerprints[i].hash == _fingerprints[j].hash) {
        fillProbeBitset(_fingerprints[i].id);
        for (; j < _fingerprints.size() && _fingerprints[i].hash == _fingerprints[j].hash; ++j) {
          if (_fingerprints[i].size == _fingerprints[j].size
              && isParallelHyperedge(_fingerprints[j].id)) {
              removeParallelHyperedge(_fingerprints[i].id, _fingerprints[j].id);
          }
        }
        i = j;
      }
    }
  }

  bool isParallelHyperedge(HyperedgeID he) const {
    bool is_parallel = true;
    forall_pins(pin, he, _hg) {
      if (!_contained_hypernodes[*pin]) {
        is_parallel = false;
        break;
      }
    } endfor
    return is_parallel;
  }

  void fillProbeBitset(HyperedgeID he) {
    _contained_hypernodes.reset();
    forall_pins(pin, he, _hg) {
      _contained_hypernodes[*pin] = 1;
    } endfor
  }

  void removeParallelHyperedge(HyperedgeID representative, HyperedgeID to_remove) {
    _hg.setEdgeWeight(representative,
                              _hg.edgeWeight(representative)
                              + _hg.edgeWeight(to_remove));
    _hg.removeEdge(to_remove);
    _removed_parallel_hyperedges.emplace_back(representative, to_remove);
    // PRINT("*** removed HE " << to_remove << " which was parallel to " << representative);
    ++_history.top().parallel_hes_size;
  }

  void createFingerprints(HypernodeID u) {
    _fingerprints.clear();
    forall_incident_hyperedges(he, u, _hg) {
      HyperedgeID hash = /* seed */ 42;
      forall_pins(pin, *he, _hg) {
        hash ^= *pin;
      } endfor
      _fingerprints.emplace_back(*he, hash, _hg.edgeSize(*he));
    } endfor
  }

  void updatePQandMappings(HypernodeID hn, const HeavyEdgeRating& rating,
                           std::vector<HypernodeID>& target, TargetToSourcesMap& sources) {
    if (rating.valid) {
      _pq.updateKey(hn, rating.value);
      if (rating.target != target[hn]) {
        updateMappings(hn, rating, target, sources);
      }
    } else if (_pq.contains(hn)) {
      _pq.remove(hn);
      removeMappingEntryOfNode(hn, target[hn], sources);
    }    
  }

  void updateMappings(HypernodeID hn, const HeavyEdgeRating& rating,
                      std::vector<HypernodeID>& target, TargetToSourcesMap& sources) {
    removeMappingEntryOfNode(hn, target[hn], sources);
    target[hn] = rating.target;
    sources.insert({rating.target, hn});
  }
  
  Hypergraph& _hg;
  const Configuration<Hypergraph>& _config;
  Rater _rater;
  std::stack<CoarseningMemento> _history;
  std::vector<HyperedgeID> _removed_single_node_hyperedges;
  std::vector<ParallelHE> _removed_parallel_hyperedges;
  std::vector<Fingerprint> _fingerprints;
  boost::dynamic_bitset<uint64_t> _contained_hypernodes;
  PriorityQueue<HypernodeID, RatingType, MetaKeyDouble> _pq;
};

} // namespace partition

#endif  // PARTITION_COARSENER_H_

/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2018 Sebastian Schlag <sebastian.schlag@kit.edu>
 * Copyright (C) 2018 Tobias Heuer <tobias.heuer@live.com>
 *
 * KaHyPar is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * KaHyPar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with KaHyPar.  If not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************************/

#pragma once

#include <algorithm>
#include <array>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "kahypar/datastructure/fast_reset_flag_array.h"
#include "kahypar/datastructure/sparse_set.h"
#include "kahypar/definitions.h"
#include "kahypar/partition/context.h"
#include "kahypar/partition/metrics.h"
#include "kahypar/utils/randomize.h"

namespace kahypar {
class QuotientGraphBlockScheduler {
  typedef std::pair<PartitionID, PartitionID> edge;
  using ConstIncidenceIterator = std::vector<edge>::const_iterator;
  using ConstCutHyperedgeIterator = std::vector<HyperedgeID>::const_iterator;

 public:
  QuotientGraphBlockScheduler(Hypergraph& hypergraph, const Context& context) :
    _hg(hypergraph),
    _context(context),
    _quotient_graph(),
    _block_pair_cut_he(context.partition.k,
                       std::vector<std::vector<HyperedgeID> >(context.partition.k,
                                                              std::vector<HyperedgeID>())),
    _visited(_hg.initialNumEdges()) { }

  QuotientGraphBlockScheduler(const QuotientGraphBlockScheduler&) = delete;
  QuotientGraphBlockScheduler(QuotientGraphBlockScheduler&&) = delete;
  QuotientGraphBlockScheduler& operator= (const QuotientGraphBlockScheduler&) = delete;
  QuotientGraphBlockScheduler& operator= (QuotientGraphBlockScheduler&&) = delete;

  void buildQuotientGraph() {
    std::set<edge> edge_list;
    for (const HyperedgeID& he : _hg.edges()) {
      if (_hg.connectivity(he) > 1) {
        for (const PartitionID& block0 : _hg.connectivitySet(he)) {
          for (const PartitionID& block1 : _hg.connectivitySet(he)) {
            if (block0 < block1) {
              edge_list.insert(std::make_pair(block0, block1));
              _block_pair_cut_he[block0][block1].push_back(he);
            }
          }
        }
      }
    }
    for (const edge& e : edge_list) {
      _quotient_graph.push_back(e);
    }
  }

  void randomShuffleQuotientEdges() {
    std::shuffle(_quotient_graph.begin(), _quotient_graph.end(), Randomize::instance().getGenerator());
  }

  std::pair<ConstIncidenceIterator, ConstIncidenceIterator> quotientGraphEdges() const {
    return std::make_pair(_quotient_graph.cbegin(), _quotient_graph.cend());
  }

  void assignBlockPairCutHyperedges(PartitionID block0, PartitionID block1, std::vector<HyperedgeID>&& cut_hes) {
    if (block1 < block0)
      std::swap(block0, block1);
    _block_pair_cut_he[block0][block1] = std::move(cut_hes);
  }

  std::pair<ConstCutHyperedgeIterator, ConstCutHyperedgeIterator> blockPairCutHyperedges(const PartitionID block0, const PartitionID block1) {
    ASSERT(block0 < block1, V(block0) << " < " << V(block1));
    updateBlockPairCutHyperedges(block0, block1);

    ASSERT([&]() {
        std::set<HyperedgeID> cut_hyperedges;
        for (const HyperedgeID& he : _block_pair_cut_he[block0][block1]) {
          if (cut_hyperedges.find(he) != cut_hyperedges.end()) {
            LOG << "Hyperedge " << he << " is contained more than once!";
            return false;
          }
          cut_hyperedges.insert(he);
        }
        for (const HyperedgeID& he : _hg.edges()) {
          if (_hg.pinCountInPart(he, block0) > 0 &&
              _hg.pinCountInPart(he, block1) > 0) {
            if (cut_hyperedges.find(he) == cut_hyperedges.end()) {
              LOG << V(_hg.pinCountInPart(he, block0));
              LOG << V(_hg.pinCountInPart(he, block1));
              LOG << V(he) << "should be inside the incidence set of"
                  << V(block0) << "and" << V(block1);
              return false;
            }
          } else {
            if (cut_hyperedges.find(he) != cut_hyperedges.end()) {
              LOG << V(_hg.pinCountInPart(he, block0));
              LOG << V(_hg.pinCountInPart(he, block1));
              LOG << V(he) << "shouldn't be inside the incidence set of"
                  << V(block0) << "and" << V(block1);
              return false;
            }
          }
        }
        return true;
      } (), "Cut hyperedge set between " << V(block0) << " and " << V(block1) << " is wrong!");

    return std::make_pair(_block_pair_cut_he[block0][block1].cbegin(),
                          _block_pair_cut_he[block0][block1].cend());
  }

  std::vector<HyperedgeID> & exposeBlockPairCutHyperedges(const PartitionID block0, const PartitionID block1) {
    updateBlockPairCutHyperedges(block0, block1);
    return _block_pair_cut_he[block0][block1];
  }

  void changeNodePart(const HypernodeID hn, const PartitionID from, const PartitionID to) {
    if (from != to) {
      _hg.changeNodePart(hn, from, to);
      for (const HyperedgeID& he : _hg.incidentEdges(hn)) {
        if (_hg.pinCountInPart(he, to) == 1) {
          for (const PartitionID& part : _hg.connectivitySet(he)) {
            if (to < part) {
              _block_pair_cut_he[to][part].push_back(he);
            } else if (to > part) {
              _block_pair_cut_he[part][to].push_back(he);
            }
          }
        }
      }
    }
  }

 private:
  static constexpr bool debug = false;

  void updateBlockPairCutHyperedges(const PartitionID block0, const PartitionID block1) {
    _visited.reset();
    size_t N = _block_pair_cut_he[block0][block1].size();
    for (size_t i = 0; i < N; ++i) {
      const HyperedgeID he = _block_pair_cut_he[block0][block1][i];
      if (_hg.pinCountInPart(he, block0) == 0 ||
          _hg.pinCountInPart(he, block1) == 0 ||
          _visited[he]) {
        std::swap(_block_pair_cut_he[block0][block1][i],
                  _block_pair_cut_he[block0][block1][N - 1]);
        _block_pair_cut_he[block0][block1].pop_back();
        --i;
        --N;
      }
      _visited.set(he, true);
    }
  }

  Hypergraph& _hg;
  const Context& _context;
  std::vector<edge> _quotient_graph;

  // Contains the cut hyperedges for each pair of blocks.
  std::vector<std::vector<std::vector<HyperedgeID> > > _block_pair_cut_he;
  ds::FastResetFlagArray<> _visited;
};
}  // namespace kahypar

/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2018 Sebastian Schlag <sebastian.schlag@kit.edu>
 * Copyright (C) 2018 Tobias Heuer <tobias.heuer@kit.edu>
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

#include "kahypar/definitions.h"
#include "kahypar/meta/policy_registry.h"
#include "kahypar/meta/typelist.h"

namespace kahypar {
class FixedVertexAcceptancePolicy : public meta::PolicyBase {
 protected:
  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE static inline bool acceptFixedVertexImbalance(const Hypergraph& hg,
                                                                                const Context& context,
                                                                                const HypernodeID u,
                                                                                const HypernodeID v) {
    if (hg.isFixedVertex(u) && hg.isFixedVertex(v)) {
      return true;
    }
    // Consider, the subhypergraph which consists of all fixed vertices.
    // The partition of this subhypergraph is given by the fixed vertex part ids.
    // If this partition is balanced, max_allowed_fixed_vertex_part_weight ensures
    // that the fixed vertex subhypergraph is balanced after coarsening phase terminates.
    // If the partition is imbalanced this upper bound implicitly force an balanced
    // partition. Keeping the fixed vertex subhypergraph balanced is very important
    // to obtain a balanced partition in recursive bisection initial partitioning mode.
    // NOTE: There are corner cases where a balanced partition of the fixed vertex
    //       subhypergraph is not possible.
    HypernodeWeight max_allowed_fixed_vertex_part_weight = (1.0 + context.partition.epsilon) *
                  ceil(static_cast<double>(hg.fixedVertexTotalWeight()) / context.partition.k);
    // Only consider the fixed vertex part weight, if u is a fixed vertex
    HypernodeWeight fixed_vertex_part_weight = hg.isFixedVertex(u) ?
                hg.fixedVertexPartWeight(hg.fixedVertexPartID(u)) : 0;
    // A contraction of two fixed vertices do not increase the fixed vertex part weight
    HypernodeWeight node_weight = hg.isFixedVertex(v) ? 0 : hg.nodeWeight(v);
    return fixed_vertex_part_weight + node_weight <= max_allowed_fixed_vertex_part_weight;
  }

  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE static inline bool acceptNumberOfFreeVertices(const Hypergraph& hg,
                                                                                const Context& context,
                                                                                const HypernodeID,
                                                                                const HypernodeID v) {
    size_t current_number_of_free_vertices = hg.currentNumNodes() - hg.numFixedVertices();
    size_t number_of_free_vertices_after = current_number_of_free_vertices - (!hg.isFixedVertex(v));
    return number_of_free_vertices_after >= context.coarsening.contraction_limit;
  }
};

class FreeContractionPartnerOnlyPolicy final : public FixedVertexAcceptancePolicy {
 public:
  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE static inline bool acceptContraction(const Hypergraph& hg,
                                                                       const Context& context,
                                                                       const HypernodeID u,
                                                                       const HypernodeID v) {
    bool accept_imbalance = acceptFixedVertexImbalance(hg, context, u, v);
    bool accept_number_of_free_vertices = acceptNumberOfFreeVertices(hg, context, u, v);
    // Hypernode v is not allowed to be a fixed vertex
    // Allowed Contractions:
    //  1.) u = Fixed <- v = Free
    //  2.) u = Free <- v = Free
    bool accept_contraction = !hg.isFixedVertex(v);
    return accept_imbalance && accept_number_of_free_vertices && accept_contraction;
  }
};

class FixedVertexContractionsAllowedPolicy final : public FixedVertexAcceptancePolicy {
 public:
  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE static inline bool acceptContraction(const Hypergraph& hg,
                                                                       const Context& context,
                                                                       const HypernodeID u,
                                                                       const HypernodeID v) {
    bool accept_imbalance = acceptFixedVertexImbalance(hg, context, u, v);
    bool accept_number_of_free_vertices = acceptNumberOfFreeVertices(hg, context, u, v);
    // Hypernode v is not allowed to be a fixed vertex
    // Allowed Contractions:
    //  1.) u = Fixed <- v = Free
    //  2.) u = Free <- v = Free
    //  3.) u = Fixed <- v = Fixed, but have to be in the same part
    bool accept_contraction = hg.isFixedVertex(u) || !hg.isFixedVertex(v);
    // If both hypernodes are fixed vertices, than they have to be in the same part
    bool accept_fixed_vertex_contraction = !(hg.isFixedVertex(u) && hg.isFixedVertex(v)) ||
                                            (hg.fixedVertexPartID(u) == hg.fixedVertexPartID(v));
    return accept_imbalance && accept_number_of_free_vertices && accept_contraction &&
           accept_fixed_vertex_contraction;
  }
};

class OnlySameTypeContractionPolicy final : public FixedVertexAcceptancePolicy {
 public:
  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE static inline bool acceptContraction(const Hypergraph& hg,
                                                                       const Context& context,
                                                                       const HypernodeID u,
                                                                       const HypernodeID v) {
    bool accept_imbalance = acceptFixedVertexImbalance(hg, context, u, v);
    bool accept_number_of_free_vertices = acceptNumberOfFreeVertices(hg, context, u, v);
    // Hypernode v is not allowed to be a fixed vertex
    // Allowed Contractions:
    //  1.) u = Fixed <- v = Fixed, but have to be in the same part
    //  2.) u = Free <- v = Free
    bool accept_contraction = (!hg.isFixedVertex(u) || hg.isFixedVertex(v)) &&
                              (hg.isFixedVertex(u) || !hg.isFixedVertex(v));
    // If both hypernodes are fixed vertices, than they have to be in the same part
    bool accept_fixed_vertex_contraction = !(hg.isFixedVertex(u) && hg.isFixedVertex(v)) ||
                                            (hg.fixedVertexPartID(u) == hg.fixedVertexPartID(v));
    return accept_imbalance && accept_number_of_free_vertices && accept_contraction &&
           accept_fixed_vertex_contraction;
  }
};


using FixedVertexAcceptancePolicies = meta::Typelist<FreeContractionPartnerOnlyPolicy,
                                                     FixedVertexContractionsAllowedPolicy,
                                                     OnlySameTypeContractionPolicy>;
}  // namespace kahypar

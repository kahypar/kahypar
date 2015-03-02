#pragma once

#include "partition/coarsening/clusterer/TwoPhaseLPClusterer.h"
#include "partition/coarsening/clusterer/LPPolicies.h"

// ugly and only for parameter tuning
//
// clique lp
template<typename SCORE, typename SIZE_CONSTR>
using two_phase_lp_clique = partition::TwoPhaseLPClusterer<
                                                  partition::OnlyLabelsInitialization,
                                                  partition::NoEdgeInitialization,
                                                  partition::DontCollectInformation,
                                                  partition::DontCollectInformation,
                                                  partition::PermutateNodes,
                                                  partition::DontPermutateLabels,
                                                  partition::AllPinsScoreComputation<SCORE>,
                                                  partition::SizeConstraintPenaltyNewLabelComputation<SIZE_CONSTR>,
                                                  partition::IgnoreGain,
                                                  partition::DontUpdateInformation,
                                                  partition::MaxIterationCondition>;

template<typename SCORE, typename SIZE_CONSTR>
using two_phase_lp_clique_sampled = partition::TwoPhaseLPClusterer<
                                                  partition::OnlyLabelsInitialization,
                                                  partition::InitializeSamplesWithUpdates,
                                                  partition::CollectInformationWithUpdates,
                                                  partition::DontCollectInformation,
                                                  partition::PermutateNodes,
                                                  partition::DontPermutateLabels,
                                                  partition::AllSampledLabelsScoreComputation<SCORE>,
                                                  partition::SizeConstraintPenaltyNewLabelComputation<SIZE_CONSTR>,
                                                  partition::IgnoreGain,
                                                  partition::UpdateInformation,
                                                  partition::MaxIterationCondition>;

template<typename SCORE, typename SIZE_CONSTR>
using two_phase_lp_clique_sampled_max_score = partition::TwoPhaseLPClusterer<
                                                  partition::OnlyLabelsInitialization,
                                                  partition::InitializeSamplesWithUpdates,
                                                  partition::CollectInformationWithUpdates,
                                                  partition::DontCollectInformation,
                                                  partition::PermutateNodes,
                                                  partition::DontPermutateLabels,
                                                  partition::AllSampledLabelsMaxScoreComputation<SCORE>,
                                                  partition::SizeConstraintPenaltyNewLabelComputation<SIZE_CONSTR>,
                                                  partition::IgnoreGain,
                                                  partition::UpdateInformation,
                                                  partition::MaxIterationCondition>;







// bipartite
template<typename SCORE, typename SIZE_CONSTR>
using bipartite_lp = partition::BipartiteLPClusterer<
                                                  partition::OnlyLabelsInitializationBipartite,
                                                  partition::PermutateNodesBipartite,
                                                  SCORE,
                                                  partition::SizeConstraintPenaltyNewLabelComputationBipartite<SIZE_CONSTR>,
                                                  partition::MaxIterationCondition>;







// sampled
template<typename SCORE, typename SIZE_CONSTR>
using two_phase_lp = partition::TwoPhaseLPClusterer<
                                                  partition::OnlyLabelsInitialization,
                                                  partition::InitializeSamplesWithUpdates,
                                                  partition::CollectInformationWithUpdates,
                                                  partition::DontCollectInformation,
                                                  partition::PermutateNodes,
                                                  partition::PermutateLabels,
                                                  partition::SampledLabelsScoreComputation<SCORE>,
                                                  partition::SizeConstraintPenaltyNewLabelComputation<SIZE_CONSTR>,
                                                  partition::DefaultGain,
                                                  partition::UpdateInformation,
                                                  partition::MaxIterationCondition>;

// sampled
template<typename SCORE, typename SIZE_CONSTR>
using two_phase_lp_max_score = partition::TwoPhaseLPClusterer<
                                                  partition::OnlyLabelsInitialization,
                                                  partition::InitializeSamplesWithUpdates,
                                                  partition::CollectInformationWithUpdates,
                                                  partition::DontCollectInformation,
                                                  partition::PermutateNodes,
                                                  partition::PermutateLabels,
                                                  partition::SampledLabelsMaxScoreComputation<SCORE>,
                                                  partition::SizeConstraintPenaltyNewLabelComputation<SIZE_CONSTR>,
                                                  partition::DefaultGain,
                                                  partition::UpdateInformation,
                                                  partition::MaxIterationCondition>;

/***************************************************************************
 *  Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#pragma once

#include <string>

namespace partition {
enum class Mode : std::uint8_t {
  recursive_bisection,
  direct_kway
};

enum class InitialPartitioningTechnique : std::uint8_t {
  multilevel,
  flat
};

enum class InitialPartitioner : std::uint8_t {
  hMetis,
  PaToH,
  KaHyPar
};

enum class CoarseningAlgorithm : std::uint8_t {
  heavy_full,
  heavy_partial,
  heavy_lazy,
  ml_style,
  hyperedge,
  do_nothing
};

enum class RefinementAlgorithm : std::uint8_t {
  twoway_fm,
  kway_fm,
  kway_fm_maxgain,
  kway_fm_km1,
  hyperedge,
  label_propagation,
  do_nothing
};

enum class InitialPartitionerAlgorithm : std::uint8_t {
  greedy_sequential,
  greedy_global,
  greedy_round,
  greedy_sequential_maxpin,
  greedy_global_maxpin,
  greedy_round_maxpin,
  greedy_sequential_maxnet,
  greedy_global_maxnet,
  greedy_round_maxnet,
  bfs,
  random,
  lp,
  pool
};

enum class RefinementStoppingRule : std::uint8_t {
  simple,
  adaptive_opt,
  adaptive1,
  adaptive2
};

enum class GlobalRebalancingMode : bool {
  off,
  on
};

enum class Objective : std::uint8_t {
  cut,
  km1
};

static std::string toString(const Mode& mode) {
  switch (mode) {
    case Mode::recursive_bisection:
      return std::string("rb");
    case Mode::direct_kway:
      return std::string("direct");
  }
  return std::string("UNDEFINED");
}

static std::string toString(const Objective& objective) {
  switch (objective) {
    case Objective::cut:
      return std::string("cut");
    case Objective::km1:
      return std::string("km1");
  }
  return std::string("UNDEFINED");
}

static std::string toString(const InitialPartitioningTechnique& technique) {
  switch (technique) {
    case InitialPartitioningTechnique::flat:
      return std::string("flat");
    case InitialPartitioningTechnique::multilevel:
      return std::string("multilevel");
  }
  return std::string("UNDEFINED");
}

static std::string toString(const InitialPartitioner& algo) {
  switch (algo) {
    case InitialPartitioner::hMetis:
      return std::string("hMetis");
    case InitialPartitioner::PaToH:
      return std::string("PaToH");
    case InitialPartitioner::KaHyPar:
      return std::string("KaHyPar");
  }
  return std::string("UNDEFINED");
}

static std::string toString(const CoarseningAlgorithm& algo) {
  switch (algo) {
    case CoarseningAlgorithm::heavy_full:
      return std::string("heavy_full");
    case CoarseningAlgorithm::heavy_partial:
      return std::string("heavy_partial");
    case CoarseningAlgorithm::heavy_lazy:
      return std::string("heavy_lazy");
    case CoarseningAlgorithm::ml_style:
      return std::string("ml_style");
    case CoarseningAlgorithm::hyperedge:
      return std::string("hyperedge");
    case CoarseningAlgorithm::do_nothing:
      return std::string("do_nothing");
  }
  return std::string("UNDEFINED");
}

static std::string toString(const RefinementAlgorithm& algo) {
  switch (algo) {
    case RefinementAlgorithm::twoway_fm:
      return std::string("twoway_fm");
    case RefinementAlgorithm::kway_fm:
      return std::string("kway_fm");
    case RefinementAlgorithm::kway_fm_maxgain:
      return std::string("kway_fm_maxgain");
    case RefinementAlgorithm::kway_fm_km1:
      return std::string("kway_fm_km1");
    case RefinementAlgorithm::hyperedge:
      return std::string("hyperedge");
    case RefinementAlgorithm::label_propagation:
      return std::string("label_propagation");
    case RefinementAlgorithm::do_nothing:
      return std::string("do_nothing");
  }
  return std::string("UNDEFINED");
}

static std::string toString(const InitialPartitionerAlgorithm& algo) {
  switch (algo) {
    case InitialPartitionerAlgorithm::greedy_sequential:
      return std::string("greedy_sequential");
    case InitialPartitionerAlgorithm::greedy_global:
      return std::string("greedy_global");
    case InitialPartitionerAlgorithm::greedy_round:
      return std::string("greedy_round");
    case InitialPartitionerAlgorithm::greedy_sequential_maxpin:
      return std::string("greedy_maxpin");
    case InitialPartitionerAlgorithm::greedy_global_maxpin:
      return std::string("greedy_global_maxpin");
    case InitialPartitionerAlgorithm::greedy_round_maxpin:
      return std::string("greedy_round_maxpin");
    case InitialPartitionerAlgorithm::greedy_sequential_maxnet:
      return std::string("greedy_maxnet");
    case InitialPartitionerAlgorithm::greedy_global_maxnet:
      return std::string("greedy_global_maxnet");
    case InitialPartitionerAlgorithm::greedy_round_maxnet:
      return std::string("greedy_round_maxnet");
    case InitialPartitionerAlgorithm::bfs:
      return std::string("bfs");
    case InitialPartitionerAlgorithm::random:
      return std::string("random");
    case InitialPartitionerAlgorithm::lp:
      return std::string("lp");
    case InitialPartitionerAlgorithm::pool:
      return std::string("pool");
  }
  return std::string("UNDEFINED");
}

static std::string toString(const RefinementStoppingRule& algo) {
  switch (algo) {
    case RefinementStoppingRule::simple:
      return std::string("simple");
    case RefinementStoppingRule::adaptive_opt:
      return std::string("adaptive_opt");
    case RefinementStoppingRule::adaptive1:
      return std::string("adaptive1");
    case RefinementStoppingRule::adaptive2:
      return std::string("adaptive2");
  }
  return std::string("UNDEFINED");
}

static std::string toString(const GlobalRebalancingMode& state) {
  switch (state) {
    case GlobalRebalancingMode::off:
      return std::string("off");
    case GlobalRebalancingMode::on:
      return std::string("on");
  }
  return std::string("UNDEFINED");
}
}  // namespace partition

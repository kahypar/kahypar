#pragma once
#include <iostream>
#include "kahypar/macros.h"
namespace kahypar {
enum class EvoReplaceStrategy : uint8_t {
  worst,
  diverse,
  strong_diverse
};

enum class EvoCombineStrategy : uint8_t {
  basic,
  edge_frequency,
  UNDEFINED
};
enum class EvoMutateStrategy : uint8_t {
  new_initial_partitioning_vcycle,
  vcycle,
  UNDEFINED
};

enum class EvoDecision :uint8_t {
  normal,
  mutation,
  combine
};


std::ostream& operator<< (std::ostream& os, const EvoReplaceStrategy& replace) {
  switch (replace) {
    case EvoReplaceStrategy::worst: return os << "worst";
    case EvoReplaceStrategy::diverse: return os << "diverse";
    case EvoReplaceStrategy::strong_diverse: return os << "strong_diverse";
      // omit default case to trigger compiler warning for missing cases
  }
  return os << static_cast<uint8_t>(replace);
}

std::ostream& operator<< (std::ostream& os, const EvoCombineStrategy& combine) {
  switch (combine) {
    case EvoCombineStrategy::basic: return os << "basic";
    case EvoCombineStrategy::edge_frequency: return os << "edge_frequency";
    case EvoCombineStrategy::UNDEFINED: return os << "-";
      // omit default case to trigger compiler warning for missing cases
  }
  return os << static_cast<uint8_t>(combine);
}

std::ostream& operator<< (std::ostream& os, const EvoMutateStrategy& mutation) {
  switch (mutation) {
    case EvoMutateStrategy::new_initial_partitioning_vcycle:
      return os << "new_initial_partitioning_vcycle";
    case EvoMutateStrategy::vcycle: return os << "vcycle";
    case EvoMutateStrategy::UNDEFINED:  return os << "-";
      // omit default case to trigger compiler warning for missing cases
  }
  return os << static_cast<uint8_t>(mutation);
}


std::ostream& operator<< (std::ostream& os, const EvoDecision& decision) {
  switch (decision) {
    case EvoDecision::normal:  return os << "normal";
    case EvoDecision::mutation:  return os << "mutation";
    case EvoDecision::combine:  return os << "combine";
      // omit default case to trigger compiler warning for missing cases
  }
  return os << static_cast<uint8_t>(decision);
}

static EvoReplaceStrategy replaceStrategyFromString(const std::string& strat) {
  if (strat == "worst") {
    return EvoReplaceStrategy::worst;
  } else if (strat == "diverse") {
    return EvoReplaceStrategy::diverse;
  } else if (strat == "strong-diverse") {
    return EvoReplaceStrategy::strong_diverse;
  }
  LOG << "No valid replace strategy. ";
  exit(0);
}
static EvoMutateStrategy mutateStrategyFromString(const std::string& strat) {
  if (strat == "new-initial-partitioning-vcycle") {
    return EvoMutateStrategy::new_initial_partitioning_vcycle;
  } else if (strat == "vcycle") {
    return EvoMutateStrategy::vcycle;
  }
  LOG << "No valid mutate strategy. ";
  exit(0);
}
static EvoCombineStrategy combineStrategyFromString(const std::string& strat) {
  if (strat == "basic") {
    return EvoCombineStrategy::basic;
  } else if (strat == "edge-frequency") {
    return EvoCombineStrategy::edge_frequency;
  }
  LOG << "No valid combine strategy. ";
  exit(0);
}
}
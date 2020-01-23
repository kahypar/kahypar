#pragma once
#include <iostream>
#include "kahypar/macros.h"
namespace kahypar {
enum class FlowAlgorithm : uint8_t {
  boykov_kolmogorov,
  ibfs,
  UNDEFINED
};

enum class FlowNetworkType : uint8_t {
  hybrid,
  UNDEFINED
};

enum class FlowExecutionMode : uint8_t {
  constant,
  multilevel,
  exponential,
  UNDEFINED
};
enum class RefinementStoppingRule : uint8_t {
  simple,
  adaptive_opt,
  UNDEFINED
};
enum class RefinementAlgorithm : uint8_t {
  twoway_fm,
  kway_fm,
  kway_fm_km1,
  twoway_flow,
  twoway_fm_flow,
  kway_flow,
  kway_fm_flow_km1,
  kway_fm_flow,
  do_nothing,
  UNDEFINED
};
std::ostream& operator<< (std::ostream& os, const FlowAlgorithm& algo) {
  switch (algo) {
    case FlowAlgorithm::boykov_kolmogorov: return os << "boykov_kolmogorov";
    case FlowAlgorithm::ibfs: return os << "ibfs";
    case FlowAlgorithm::UNDEFINED: return os << "UNDEFINED";
      // omit default case to trigger compiler warning for missing cases
  }
  return os << static_cast<uint8_t>(algo);
}

std::ostream& operator<< (std::ostream& os, const FlowNetworkType& type) {
  switch (type) {
    case FlowNetworkType::hybrid: return os << "hybrid";
    case FlowNetworkType::UNDEFINED: return os << "UNDEFINED";
      // omit default case to trigger compiler warning for missing cases
  }
  return os << static_cast<uint8_t>(type);
}

std::ostream& operator<< (std::ostream& os, const FlowExecutionMode& mode) {
  switch (mode) {
    case FlowExecutionMode::constant: return os << "constant";
    case FlowExecutionMode::multilevel: return os << "multilevel";
    case FlowExecutionMode::exponential: return os << "exponential";
    case FlowExecutionMode::UNDEFINED: return os << "UNDEFINED";
      // omit default case to trigger compiler warning for missing cases
  }
  return os << static_cast<uint8_t>(mode);
}


std::ostream& operator<< (std::ostream& os, const RefinementAlgorithm& algo) {
  switch (algo) {
    case RefinementAlgorithm::twoway_fm: return os << "twoway_fm";
    case RefinementAlgorithm::kway_fm: return os << "kway_fm";
    case RefinementAlgorithm::kway_fm_km1: return os << "kway_fm_km1";
    case RefinementAlgorithm::twoway_flow: return os << "twoway_flow";
    case RefinementAlgorithm::twoway_fm_flow: return os << "twoway_fm_flow";
    case RefinementAlgorithm::kway_flow: return os << "kway_flow";
    case RefinementAlgorithm::kway_fm_flow_km1: return os << "kway_fm_flow_km1";
    case RefinementAlgorithm::kway_fm_flow: return os << "kway_fm_flow";
    case RefinementAlgorithm::do_nothing: return os << "do_nothing";
    case RefinementAlgorithm::UNDEFINED: return os << "UNDEFINED";
      // omit default case to trigger compiler warning for missing cases
  }
  return os << static_cast<uint8_t>(algo);
}



std::ostream& operator<< (std::ostream& os, const RefinementStoppingRule& rule) {
  switch (rule) {
    case RefinementStoppingRule::simple: return os << "simple";
    case RefinementStoppingRule::adaptive_opt: return os << "adaptive_opt";
    case RefinementStoppingRule::UNDEFINED: return os << "UNDEFINED";
      // omit default case to trigger compiler warning for missing cases
  }
  return os << static_cast<uint8_t>(rule);
}




static RefinementStoppingRule stoppingRuleFromString(const std::string& rule) {
  if (rule == "simple") {
    return RefinementStoppingRule::simple;
  } else if (rule == "adaptive_opt") {
    return RefinementStoppingRule::adaptive_opt;
  }
  LOG << "No valid stopping rule for FM.";
  exit(0);
  return RefinementStoppingRule::simple;
}
static RefinementAlgorithm refinementAlgorithmFromString(const std::string& type) {
  if (type == "twoway_fm") {
    return RefinementAlgorithm::twoway_fm;
  } else if (type == "kway_fm") {
    return RefinementAlgorithm::kway_fm;
  } else if (type == "kway_fm_km1") {
    return RefinementAlgorithm::kway_fm_km1;
  } else if (type == "twoway_flow") {
    return RefinementAlgorithm::twoway_flow;
  } else if (type == "twoway_fm_flow") {
    return RefinementAlgorithm::twoway_fm_flow;
  } else if (type == "kway_flow") {
    return RefinementAlgorithm::kway_flow;
  } else if (type == "kway_fm_flow_km1") {
    return RefinementAlgorithm::kway_fm_flow_km1;
  } else if (type == "kway_fm_flow") {
    return RefinementAlgorithm::kway_fm_flow;
  } else if (type == "do_nothing") {
    return RefinementAlgorithm::do_nothing;
  }
  LOG << "Illegal option:" << type;
  exit(0);
  return RefinementAlgorithm::kway_fm;
}

static FlowAlgorithm flowAlgorithmFromString(const std::string& type) {
  if (type == "boykov_kolmogorov") {
    return FlowAlgorithm::boykov_kolmogorov;
  } else if (type == "ibfs") {
    return FlowAlgorithm::ibfs;
  }
  LOG << "Illegal option:" << type;
  exit(0);
  return FlowAlgorithm::ibfs;
}

static FlowNetworkType flowNetworkFromString(const std::string& type) {
  if (type == "hybrid") {
    return FlowNetworkType::hybrid;
  }
  LOG << "No valid flow network type.";
  exit(0);
  return FlowNetworkType::hybrid;
}

static FlowExecutionMode flowExecutionPolicyFromString(const std::string& mode) {
  if (mode == "constant") {
    return FlowExecutionMode::constant;
  } else if (mode == "multilevel") {
    return FlowExecutionMode::multilevel;
  } else if (mode == "exponential") {
    return FlowExecutionMode::exponential;
  }
  LOG << "No valid flow execution mode.";
  exit(0);
  return FlowExecutionMode::exponential;
}
}
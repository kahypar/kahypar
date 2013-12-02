#ifndef LIB_DEFINITIONS_H_
#define LIB_DEFINITIONS_H_

#include <vector>

namespace defs {
typedef unsigned int HyperNodeID;
typedef unsigned int HyperEdgeID;
typedef unsigned int HyperNodeWeight;
typedef unsigned int HyperEdgeWeight;

typedef unsigned int PartitionID;

typedef double RatingType;

typedef std::vector<size_t> hMetisHyperEdgeIndexVector;
typedef std::vector<HyperNodeID> hMetisHyperEdgeVector;
typedef std::vector<HyperNodeWeight> hMetisHyperNodeWeightVector;
typedef std::vector<HyperEdgeWeight> hMetisHyperEdgeWeightVector;
} // namespace defs
#endif  // LIB_DEFINITIONS_H_

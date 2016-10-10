#pragma once

#include "kahypar/datastructure/hash_table.h"
#include "kahypar/definitions.h"
#include "kahypar/utils/hash_vector.h"

#include <algorithm>
#include <limits>
#include <vector>

namespace kahypar {

template<typename _HashFunc>
class MinHashPolicy {
public:
  using HashFunc = _HashFunc;
  using VertexId = Hypergraph::HypernodeID;
  using HashValue = typename HashFunc::HashValue;
  using MyHashSet = HashSet<HashValue>;
  using VertexSet = std::vector<VertexId>;

  explicit MinHashPolicy(uint32_t hashNum = 0, uint32_t seed = 0)
          : _dim(hashNum), _seed(seed), _hashFuncVector(hashNum, seed) {}

  void reset(uint32_t seed) {
    _seed = seed;
    _hashFuncVector.reset(_seed);
  }

  void reset(uint32_t seed, uint32_t hashNum) {
    _dim = hashNum;
    _hashFuncVector.resize(hashNum);

    reset(seed);
  }

  // calculates minHashes for vertices in [begin, end)
  void operator()(const Hypergraph &graph, VertexId begin, VertexId end, MyHashSet &hashSet) const {
    ASSERT(getHashNum() > 0, "The number of hashes should be greater than zero");
    for (auto vertexId = begin; vertexId != end; ++vertexId) {
      for (uint32_t hashNum = 0; hashNum < _hashFuncVector.getHashNum(); ++hashNum) {
        hashSet[hashNum][vertexId] = minHash(_hashFuncVector[hashNum], graph.incidentEdges(vertexId));
      }
    }
  }

  void calculateLastHash(const Hypergraph &graph, const VertexSet &vertices,
                         MyHashSet &hashSet) const {
    ASSERT(getHashNum() > 0, "The number of hashes should be greater than zero");
    for (auto vertexId : vertices) {
      size_t lastHash = _hashFuncVector.getHashNum() - 1;
      hashSet[lastHash][vertexId] = minHash(_hashFuncVector[lastHash], graph.incidentEdges(vertexId));
    }
  }

  template<typename Iterator>
  void calculateLastHash(const Hypergraph &graph, Iterator begin, Iterator end,
                         MyHashSet &hashSet) const {
    ASSERT(getHashNum() > 0, "The number of hashes should be greater than zero");
    for (auto it = begin; it != end; ++it) {
      size_t lastHash = _hashFuncVector.getHashNum() - 1;
      hashSet[lastHash][*it] = minHash(_hashFuncVector[lastHash], graph.incidentEdges(*it));
    }
  }

  HashValue combinedHash(const Hypergraph &graph, VertexId vertexId) const {
    ASSERT(getHashNum() > 0, "The number of hashes should be greater than zero");
    HashValue val = 0;
    for (uint32_t hashNum = 0; hashNum < _hashFuncVector.getHashNum(); ++hashNum) {
      val ^= minHash(_hashFuncVector[hashNum], graph.incidentEdges(vertexId));
    }
    return val;
  }

  uint32_t getDim() const {
    return _dim;
  }

  uint32_t getHashNum() const {
    return _hashFuncVector.getHashNum();
  }

  void setHashNumAndDim(uint32_t hashNum) {
    _dim = hashNum;
    _hashFuncVector.Resize(hashNum);
  }

  void addHashFunction(uint32_t seed) {
    _hashFuncVector.addHashFunction(seed);
    ++_dim;
  }

  void reserveHashFunctions(uint32_t size) {
    _hashFuncVector.reserve(size);
  }

protected:
  // The number of dimensions for buckets where we put hash values.
  // In this policy we put a hash value of ith hash function to ith dimension
  uint32_t _dim;

  uint32_t _seed;

  HashFuncVector<HashFunc> _hashFuncVector;

  // calculates minHash from incident edges of a particular vertex
  template<typename ValuesRange>
  HashValue minHash(HashFunc hash, const ValuesRange &valuesRange) const {
    HashValue res = std::numeric_limits<HashValue>::max();

    for (auto value : valuesRange) {
      res = std::min(res, hash(value));
    }
    return res;
  }

};

using MinMurmurHashPolicy = MinHashPolicy<math::MurmurHash<uint32_t>>;

template<typename _THashFunc>
class CombinedHashPolicy {
public:
  using BaseHashPolicy = _THashFunc;
  using VertexId = Hypergraph::HypernodeID;
  using HashValue = typename BaseHashPolicy::HashValue;
  using MyHashSet = HashSet<HashValue>;

  CombinedHashPolicy(uint32_t hashNum, uint32_t combinedHashNum, uint32_t seed)
          : _seed(seed), _dim(hashNum), _hashNum(hashNum), _combinedHashNum(combinedHashNum) {}

  // calculates minHashes for vertices in [begin, end)
  void operator()(const Hypergraph &graph, VertexId begin, VertexId end, MyHashSet &hashSet) const {
    ASSERT(getHashNum() > 0, "The number of hashes should be greater than zero");
    HashFuncVector <BaseHashPolicy> hashFuncVectors(_hashNum);

    hashFuncVectors.Reset(_seed, _combinedHashNum);

    for (auto vertexId = begin; vertexId != end; ++vertexId) {
      for (uint32_t hashNum = 0; hashNum < _hashNum; ++hashNum) {
        hashSet[hashNum][vertexId] = hashFuncVectors[hashNum].combinedHash(graph, vertexId);
      }
    }
  }

  uint32_t getDim() const {
    return _dim;
  }

  uint32_t getHashNum() const {
    return _hashNum;
  }

  uint32_t getCombinedHashNum() const {
    return _combinedHashNum;
  }

  void setHashNumAndDim(uint32_t hashNum) {
    _dim = hashNum;
    _hashNum = hashNum;
  }

  void setCombinedHashNum(uint32_t combinedHashNum) {
    _combinedHashNum = combinedHashNum;
  }

private:
  uint32_t _seed;

  // The number of dimensions for buckets where we put hash values.
  // In this policy we put a hash value of ith hash function to ith dimension
  uint32_t _dim;

  // The number of hash functions
  uint32_t _hashNum;

  // The number of combined hash functions
  uint32_t _combinedHashNum;
};

using LSHCombinedHashPolicy = CombinedHashPolicy<MinMurmurHashPolicy>;

template<typename T>
class CombinedHashNum {
public:
  static uint32_t get(T &) {
    return 0;
  }

  static void set(T &, uint32_t) {
  }
};

template<>
class CombinedHashNum<LSHCombinedHashPolicy> {
public:
  static uint32_t get(LSHCombinedHashPolicy &policy) {
    return policy.getCombinedHashNum();
  }

  static void set(LSHCombinedHashPolicy &policy, uint32_t combinedHashNum) {
    policy.setCombinedHashNum(combinedHashNum);
  }
};

class Coarsening {
public:
  using VertexId = HypernodeID;
  using EdgeId = HyperedgeID;

  static Hypergraph build(const Hypergraph &hypergraph, std::vector<VertexId> &clusters, PartitionID partitionId) {
    EdgeId numEdges = hypergraph.currentNumEdges();

    // 0 : reenumerate
    VertexId numVertices = reenumerateClusters(clusters);

    // 1 : build vector of indicies and pins for hyperedges
    std::vector<size_t> indicesOfEdges;
    indicesOfEdges.reserve(numEdges + 1);

    std::vector<VertexId> pinsOfEdges;
    pinsOfEdges.reserve(hypergraph.currentNumPins());

    hash_map_no_erase<Edge, std::pair<EdgeId, HyperedgeWeight>, HashEdge, false> parallelEdges(3 * numEdges);

    size_t offset = 0;
    size_t removedEdges = 0;
    size_t nonDisabledEdgeId = 0;
    for (auto edgeId : hypergraph.edges()) {
      auto pinsRange = hypergraph.pins(edgeId);
      hash_set_no_erase<VertexId> newPins(pinsRange.second - pinsRange.first);

      for (auto vertexId : pinsRange) {
        newPins.insert(clusters[vertexId]);
      }

      // if we have only one vertex in a hyperedge then we remove the hyperedge
      if (newPins.size() <= 1) {
        ++removedEdges;
        ++nonDisabledEdgeId;
        continue;
      }

      for (auto newPin : newPins) {
        pinsOfEdges.push_back(newPin);
      }

      // check for parallel edges
      Edge edge(pinsOfEdges.begin() + offset, pinsOfEdges.begin() + offset + newPins.size());

      if (!parallelEdges.contains(edge)) {
        // no parallel edges
        indicesOfEdges.push_back(offset);
        offset += newPins.size();
        parallelEdges.insert(std::make_pair(edge, std::make_pair(nonDisabledEdgeId - removedEdges,
                                                                 hypergraph.edgeWeight(edgeId))));
      } else {
        ++parallelEdges[edge].second;

        ++removedEdges;
        pinsOfEdges.resize(pinsOfEdges.size() - newPins.size());
      }
      ++nonDisabledEdgeId;
    }
    indicesOfEdges.push_back(offset);

    // 2 : calculate weights of vertices in contracted graph
    std::vector<HypernodeWeight> vertexWeights(numVertices);

    for (auto clusterId : clusters) {
      ++vertexWeights[clusterId];
    }

    // 3 : calculate weights of hyper edgs in contracted graph
    numEdges -= removedEdges;
    std::vector<HyperedgeWeight> edgeWeights(numEdges);
    for (const auto &value : parallelEdges) {
      edgeWeights[value.second.first] = value.second.second;
    }

    return Hypergraph(numVertices, numEdges, indicesOfEdges, pinsOfEdges, partitionId, &edgeWeights, &vertexWeights);
  }

private:
  using IncidenceIterator = Hypergraph::IncidenceIterator;

  static VertexId reenumerateClusters(std::vector<VertexId>& clusters) {
    if (clusters.empty())
      return 0;

    std::vector<VertexId> newClusters(clusters.size());

    for (auto clst : clusters)
      newClusters[clst] = 1;

    for (size_t i = 1; i < newClusters.size(); ++i)
      newClusters[i] += newClusters[i - 1];

    for (size_t node = 0; node < clusters.size(); ++node)
      clusters[node] = newClusters[clusters[node]] - 1;

    return newClusters.back();
  }

  struct Edge {
    IncidenceIterator Begin, End;

    Edge() {
    }

    Edge(IncidenceIterator begin, IncidenceIterator end)
            : Begin(begin), End(end) {

    }

    IncidenceIterator begin() const {
      return Begin;
    }

    IncidenceIterator end() const {
      return End;
    }

    bool operator==(const Edge &edge) const {
      size_t thisSize = End - Begin;
      size_t otherSize = edge.End - edge.Begin;

      if (thisSize != otherSize) {
        return false;
      }

      for (size_t offset = 0; offset < thisSize; ++offset) {
        if (*(Begin + offset) != *(edge.Begin + offset)) {
          return false;
        }
      }

      return true;
    }

    bool operator<(const Edge &edge) const {
      size_t thisSize = End - Begin;
      size_t otherSize = edge.End - edge.Begin;

      size_t size = std::min(thisSize, otherSize);

      for (size_t offset = 0; offset < size; ++offset) {
        if (*(Begin + offset) < *(edge.Begin + offset)) {
          return true;
        } else {
          if (*(Begin + offset) > *(edge.Begin + offset)) {
            return false;
          }
        }
      }
      return thisSize < otherSize;
    }
  };

  struct HashEdge {
    size_t operator()(const Edge &edge) const {
      size_t hash = 0;
      math::MurmurHash <uint32_t> hashFunc;
      for (auto edgeId : edge) {
        hash ^= hashFunc(edgeId);
      }
      return hash;
    }
  };
};

class Uncoarsening {
public:
  using VertexId = HypernodeID;

  static void applyPartition(const Hypergraph &coarsanedGraph, std::vector<VertexId> &clusters,
                             Hypergraph &originalGraph) {
    for (auto vertexId : originalGraph.nodes()) {
      originalGraph.setNodePart(vertexId, coarsanedGraph.partID(clusters[vertexId]));
    }
  }
};

}

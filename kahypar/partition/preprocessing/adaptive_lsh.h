#pragma once

#include "kahypar/datastructure/hash_table.h"
#include "kahypar/partition/preprocessing/lsh_utils.h"
#include "kahypar/utils/stats.h"

#include <algorithm>
#include <queue>

namespace kahypar {

template<typename _HashPolicy>
class AdaptiveLSHWithConnectedComponents {
private:
  using VertexId = Hypergraph::HypernodeID;
  using HashPolicy = _HashPolicy;
  using BaseHashPolicy = typename HashPolicy::BaseHashPolicy;
  using HashValue = typename HashPolicy::HashValue;

public:

  explicit AdaptiveLSHWithConnectedComponents(const Hypergraph &hypergraph,
                                              std::unique_ptr<HashPolicy> hashPolicy,
                                              const uint32_t seed,
                                              const uint32_t maxHyperedgeSize,
                                              const uint32_t maxClusterSize,
                                              const uint32_t minClusterSize,
                                              const bool collectStats)
          : _hypergraph(hypergraph),
            _hashPolicy(std::move(hashPolicy)),
            _seed(seed),
            _maxHyperedgeSize(maxHyperedgeSize),
            _maxClusterSize(maxClusterSize),
            _minClusterSize(minClusterSize),
            _maxNumHashFunc(getHashNum()),
            _maxCombinedNumHashFunc(getCombinedHashNum()),
            _collectStats(collectStats)
  {}

  std::vector<VertexId> build() {
    return adaptiveWhole();
  }

  uint32_t getHashNum() const {
    return _hashPolicy->getHashNum();
  }

  uint32_t getDim() const {
    return _hashPolicy->getDim();
  }

  uint32_t getCombinedHashNum() const {
    return CombinedHashNum<HashPolicy>::get(*_hashPolicy);
  }

private:
  using MyHashSet = HashSet<HashValue>;
  using Buckets = HashBuckets<HashValue, VertexId>;
  using VertexWeight = Hypergraph::HypernodeWeight;

  const Hypergraph &_hypergraph;
  std::unique_ptr<HashPolicy> _hashPolicy;
  const uint32_t _seed;
  const uint32_t _maxHyperedgeSize;
  const uint32_t _maxClusterSize;
  const uint32_t _minClusterSize;
  const uint32_t _maxNumHashFunc;
  const uint32_t _maxCombinedNumHashFunc;
  const bool _collectStats;

  std::vector<VertexId> adaptiveWhole() {
    std::default_random_engine eng(_seed);
    std::uniform_int_distribution<uint32_t> rnd;

    MyHashSet mainHashSet(0, _hypergraph.currentNumNodes());
    mainHashSet.reserve(20);

    std::vector<VertexId> clusters;
    clusters.reserve(_hypergraph.currentNumNodes());

    std::vector<uint32_t> clusterSize(_hypergraph.currentNumNodes(), 1);

    std::vector<uint8_t> activeClustersBoolSet(_hypergraph.currentNumNodes(), true);
    uint32_t numActiveVertices = _hypergraph.currentNumNodes();

    for (VertexId vertexId = 0; vertexId < _hypergraph.currentNumNodes(); ++vertexId) {
      clusters.push_back(vertexId);
    }

    std::vector<VertexId> inactiveClusters;
    inactiveClusters.reserve(_hypergraph.currentNumNodes());

    while (numActiveVertices > 0 && mainHashSet.getHashNum() < _maxNumHashFunc) {
      mainHashSet.addHashVector();

      std::vector<VertexId> activeVerticesSet;
      activeVerticesSet.reserve(numActiveVertices);

      for (VertexId vertexId = 0; vertexId < _hypergraph.currentNumNodes(); ++vertexId) {
        VertexId cluster = clusters[vertexId];
        if (activeClustersBoolSet[cluster]) {
          activeVerticesSet.push_back(vertexId);
        }
      }

      uint32_t hashNum = mainHashSet.getHashNum() - 1;
      auto start = std::chrono::high_resolution_clock::now();
      incrementalParametersEstimation(activeVerticesSet, rnd(eng), _maxClusterSize, mainHashSet, hashNum);
      auto end = std::chrono::high_resolution_clock::now();
      Stats::instance().addToTotal(_collectStats, "Adaptive LSH: Incremental parameter estimation",
                                   std::chrono::duration<double>(end - start).count());

      Buckets buckets(1, _hypergraph.currentNumNodes());
      start = std::chrono::high_resolution_clock::now();
      calculateOneDimBucket(_hypergraph.currentNumNodes(), activeClustersBoolSet, clusters, mainHashSet,
                            buckets, hashNum);
      end = std::chrono::high_resolution_clock::now();
      Stats::instance().addToTotal(_collectStats, "Adaptive LSH: Construction of buckets",
                                   std::chrono::duration<double>(end - start).count());

      start = std::chrono::high_resolution_clock::now();
      calculateClustersIncrementally(_hypergraph.currentNumNodes(), activeClustersBoolSet, clusters, clusterSize,
                                     mainHashSet, hashNum, buckets, inactiveClusters);
      end = std::chrono::high_resolution_clock::now();
      Stats::instance().addToTotal(_collectStats, "Adaptive LSH: Construction of clustering",
                                   std::chrono::duration<double>(end - start).count());

      std::vector<char> bitMap(clusters.size());
      for (auto clst : clusters)
        bitMap[clst] = 1;

      size_t numCl = 0;
      for (auto bit : bitMap) {
        if (bit) {
          ++numCl;
        }
      }
      LOG("Num clusters: " << numCl);


      std::sort(inactiveClusters.begin(), inactiveClusters.end());
      auto endIter = std::unique(inactiveClusters.begin(), inactiveClusters.end());
      inactiveClusters.resize(endIter - inactiveClusters.begin());

      for (auto cluster : inactiveClusters) {
        activeClustersBoolSet[cluster] = false;
        numActiveVertices -= clusterSize[cluster];
      }
      inactiveClusters.clear();

      if (numCl <= _hypergraph.currentNumNodes() / 2) {
        LOG("Adaptively chosen number of hash functions: " << mainHashSet.getHashNum());
        break;
      }
    }

    return clusters;
  }

  struct VecHash {
    size_t operator()(const std::vector<uint64_t> &vec) const {
      size_t res = 0;
      for (auto el : vec) {
        res ^= el;
      }

      return res;
    }
  };

  void incrementalParametersEstimation(std::vector<VertexId> &activeVertices, uint32_t seed,
                                       uint32_t bucketMinSize, MyHashSet &mainHashSet, uint32_t mainHashNum) {

    MyHashSet hashSet(0, _hypergraph.currentNumNodes());
    hashSet.reserve(_maxCombinedNumHashFunc);

    BaseHashPolicy baseHashPolicy(0, seed);
    baseHashPolicy.reserveHashFunctions(_maxCombinedNumHashFunc);

    std::default_random_engine eng(seed);
    std::uniform_int_distribution<uint32_t> rnd;

    // in the beginning all vertices are in the same bucket (same hash)
    std::vector<HashValue> hashes(_hypergraph.currentNumNodes());

    size_t remainedVertices = activeVertices.size();

    size_t bucketsNum = _hypergraph.currentNumNodes();

    using TPair = std::pair<HashValue, VertexId>;

    std::vector<TPair> buckets;
    buckets.reserve(bucketsNum);

    // empirically best value
    uint32_t minHashNum = 10;

    uint32_t maxBucketSize = _maxClusterSize;
    ASSERT(minHashNum <= _maxCombinedNumHashFunc,
           "# min combined hash funcs should be <= # max combined hash funcs");
    std::vector<TPair> newBuckets;
    newBuckets.reserve(bucketsNum);

    for (size_t i = 0; i + 1 < minHashNum; ++i) {
      hashSet.addHashVector();
      baseHashPolicy.addHashFunction(rnd(eng));

      baseHashPolicy.calculateLastHash(_hypergraph, activeVertices, hashSet);

      uint32_t lastHash = hashSet.getHashNum() - 1;
      for (auto ver : activeVertices) {
        hashes[ver] ^= hashSet[lastHash][ver];
      }
    }

    for (auto ver : activeVertices) {
      buckets.emplace_back(hashes[ver], ver);
    }
    std::sort(buckets.begin(), buckets.end());

    while (remainedVertices > 0) {
      hashSet.addHashVector();
      baseHashPolicy.addHashFunction(rnd(eng));

      uint32_t lastHash = hashSet.getHashNum() - 1;

      // Decide for which vertices we continue to increase the number of hash functions
      newBuckets.clear();

      auto begin = buckets.begin();

      while (begin != buckets.end()) {
        auto end = begin;
        while (end != buckets.end() && begin->first == end->first) {
          ++end;
        }
        std::vector<VertexId> vertices;
        vertices.reserve(end - begin);
        for (auto it = begin; it != end; ++it) {
          vertices.push_back(it->second);
        }

        baseHashPolicy.calculateLastHash(_hypergraph, vertices, hashSet);

        if (vertices.size() == 1) {
          --remainedVertices;
          HashValue hash = hashSet[lastHash][vertices.front()];
          hashes[vertices.front()] ^= hash;
          begin = end;
          continue;
        }

        std::vector<TPair> newHashes;
        newHashes.reserve(vertices.size());

        for (auto vertex : vertices) {

          HashValue hash = hashSet[lastHash][vertex];
          hashes[vertex] ^= hash;

          newHashes.emplace_back(hashes[vertex], vertex);
        }

        std::sort(newHashes.begin(), newHashes.end());
        auto e = std::unique(newHashes.begin(), newHashes.end());
        newHashes.resize(e - newHashes.begin());

        auto newBegin = newHashes.begin();
        while (newBegin != newHashes.end()) {
          auto newEnd = newBegin;
          while (newEnd != newHashes.end() && newBegin->first == newEnd->first) {
            ++newEnd;
          }
          VertexId bucketSize = newEnd - newBegin;
          if ((bucketSize <= maxBucketSize && hashSet.getHashNum() >= minHashNum)
              || hashSet.getHashNum() >= _maxCombinedNumHashFunc
                  ) {
            remainedVertices -= bucketSize;
          } else {
            std::copy(newBegin, newEnd, std::back_inserter(newBuckets));
          }
          newBegin = newEnd;
        }
        begin = end;
      }
      buckets.swap(newBuckets);
    }
    for (uint32_t vertexId = 0; vertexId < _hypergraph.currentNumNodes(); ++vertexId) {
      mainHashSet[mainHashNum][vertexId] = hashes[vertexId];
    }
  }

  // distributes vertices among bucket according to the new hash values
  void calculateOneDimBucket(VertexId numVertex, const std::vector<uint8_t> &activeClustersBoolSet,
                             const std::vector<VertexId> &clusters, const MyHashSet &hashSet, Buckets &buckets,
                             uint32_t hashNum) {

    ASSERT(buckets.isOneDimensional(), "Bucket should be one dimensional");
    uint32_t dim = 0;

    for (VertexId vertexId = 0; vertexId < numVertex; ++vertexId) {
      VertexId cluster = clusters[vertexId];
      if (activeClustersBoolSet[cluster]) {
        buckets.put(dim, hashSet[hashNum][vertexId], vertexId);
      }
    }
  }

  // incrementally calculates clusters according to the new bucket
  void calculateClustersIncrementally(VertexId numVertex, std::vector<uint8_t> &activeClustersBoolSet,
                                      std::vector<VertexId> &clusters, std::vector<uint32_t> &clusterSize,
                                      const MyHashSet &hashSet, uint32_t hashNum, Buckets &buckets,
                                      std::vector<VertexId> &inactiveClusters) {
    // Calculate clusters according to connected components
    // of an implicit graph induced by buckets. We use BFS on this graph.
    runIncrementalBfs(numVertex, activeClustersBoolSet, hashSet, buckets, hashNum, clusters, clusterSize,
                      inactiveClusters);
  }

  void runIncrementalBfs(VertexId numVertex, std::vector<uint8_t> &activeClustersBoolSet,
                         const MyHashSet &hashSet, Buckets &buckets, uint32_t hashNum,
                         std::vector<VertexId> &clusters, std::vector<uint32_t> &clusterSize,
                         std::vector<VertexId> &inactiveClusters) {
    std::queue<VertexId> vertexQueue;
    std::vector<char> visited(numVertex, false);
    for (VertexId vertexId = 0; vertexId < numVertex; ++vertexId) {
      VertexId cluster = clusters[vertexId];
      if (!visited[vertexId] && activeClustersBoolSet[cluster]) {
        visited[vertexId] = true;
        vertexQueue.push(vertexId);
        runIncrementalBfs(vertexId, activeClustersBoolSet, hashSet, buckets, visited,
                          hashNum, clusters, clusterSize, inactiveClusters);
      }
    }
  }

  void runIncrementalBfs(VertexId curVertex, std::vector<uint8_t> &activeClustersBoolSet, const MyHashSet &hashSet,
                         Buckets &buckets, std::vector<char> &visited,
                         uint32_t hashNum, std::vector<VertexId> &clusters, std::vector<uint32_t> &clusterSize,
                         std::vector<VertexId> &inactiveClusters
  ) {

    VertexId curCluster = clusters[curVertex];
    HashValue hash = hashSet[hashNum][curVertex];
    uint32_t dim = buckets.isOneDimensional() ? 0 : hashNum;

    std::vector<VertexId> neighbours;
    neighbours.reserve(_maxHyperedgeSize);

    auto iters = buckets.getObjects(dim, hash);
    for (auto neighbourIter = iters.first; neighbourIter != iters.second; ++neighbourIter) {
      auto neighbour = *neighbourIter;
      VertexId neighbourCluster = clusters[neighbour];
      if (activeClustersBoolSet[neighbourCluster]) {
        VertexWeight weight = _hypergraph.nodeWeight(neighbour);
        if (clusterSize[curCluster] + weight > _maxClusterSize) {
          ASSERT(clusterSize[curCluster] <= _maxClusterSize, "Cluster is overflowed");
          break;
        }

        clusterSize[neighbourCluster] -= weight;
        clusters[neighbour] = curCluster;
        clusterSize[curCluster] += weight;

        if (clusterSize[curCluster] >= _minClusterSize) {
          inactiveClusters.push_back(curCluster);
          activeClustersBoolSet[curCluster] = false;
        }

        visited[neighbour] = true;
        neighbours.push_back(neighbour);
      }
    }

    for (auto neighbour : neighbours) {
      uint32_t dim = buckets.isOneDimensional() ? 0 : hashNum;
      buckets.removeObject(dim, hashSet[hashNum][neighbour], neighbour);
    }
  }
};

using MinHashSparsifier = AdaptiveLSHWithConnectedComponents<LSHCombinedHashPolicy>;

}

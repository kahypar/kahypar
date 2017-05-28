
namespace kahypar {
namespace combine {
namespace edgefrequency {
  std::vector<std::size_t> frequencyFromPopulation(const Context& context, const std::vector<Individual>& edgeFreqTargets, const std::size_t& size) {
    std::vector<std::size_t> returnVector(size);
    for(std::size_t i = 0; i < edgeFreqTargets.size(); ++i) {
      std::vector<HyperedgeID> currentCutEdges = edgeFreqTargets[i].cutEdges();
      for(std::size_t j = 0; j < currentCutEdges.size(); ++j) {
        returnVector[currentCutEdges[j]] += 1;
      }
    }
    return returnVector;
  }
}
}
}

#include "kahypar/definitions.h"
namespace kahypar {
namespace combine {
namespace stablenet {
  static std::vector<HyperedgeID> stableNetsFromMultipleIndividuals(const Context& context,const std::vector<Individual>& individuals, const std::size_t& size) {
    //const std::vector<std::size_t> frequency = kahypar::combine::edgefrequency::frequencyFromPopulation(context, individuals, size);
    std::vector<HyperedgeID> stableNetEdges;
    /*for(HyperedgeID i = 0; i < frequency.size(); ++i) {
      if(frequency[i] >= context.evolutionary.stable_net_amount * individuals.size()) {
        stableNetEdges.push_back(i);
      }
    }*/
    return stableNetEdges;
  }
}
}
}

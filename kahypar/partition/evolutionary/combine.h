namespace kahypar {
namespace combine {
  Individual partitions(const Context& context);
  Individual crossCombine(const Context& context);
  Individual edgeFrequency(const Context& context);
  Individual edgeFrequencyWithAdditionalPartitionInformation(const Context& context);
  Individual populationStableNet(const Context& context);
  Individual populationStableNetWithAdditionalPartitionInformation(const Context& context);
}//namespace combine
}//namespace kahypar

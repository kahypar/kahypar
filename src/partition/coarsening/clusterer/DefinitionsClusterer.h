#pragma once

namespace partition
{
  using Label = uint32_t;
  struct NodeData
  {
    NodeData() : label(0), location_incident_edges_incident_labels() {};
    Label label;
    std::vector<int> location_incident_edges_incident_labels;
  };

  struct EdgeData
  {
    //Label label;
    std::vector<std::pair<Label, defs::PartitionID> > incident_labels;
    std::vector<int> location; // location of the incident_labels in the sample

    bool small_edge;

    std::pair<Label, defs::PartitionID> *sampled;
    uint32_t sample_size;

    //std::vector<bool> sampled_labels;
    //size_t sampled_labels_count; // how many labels were sampled

    std::unordered_map<Label, uint32_t> label_count_map; // map of Labels -> number of this labels in this edge
    size_t count_labels; // how many different labels does this Hyperedge have (connectivity)

    EdgeData() :
      incident_labels(),
      location(),
      small_edge(false),
      sampled(nullptr),
      sample_size(0),
      label_count_map(),
      count_labels(1) // ugly hack!
    {
    }

    ~EdgeData()
    {
      if (!small_edge && sampled != nullptr)
      {
        delete[] sampled;
      }
    }

    EdgeData& operator=(const EdgeData &edge_data)
    {
      if (!small_edge && sampled != nullptr)
      {
        delete[] sampled;
      }

      incident_labels = edge_data.incident_labels;
      location = edge_data.location;
      small_edge = edge_data.small_edge;
      sample_size = edge_data.sample_size;
      label_count_map = edge_data.label_count_map;

      if (small_edge)
      {
        sampled = incident_labels.data();
      } else {
        sampled = new std::pair<Label, defs::PartitionID>[sample_size];
      }

      return *this;
    }

    EdgeData& operator=(EdgeData &&edge_data)
    {
      if (!small_edge && sampled != nullptr)
      {
        delete[] sampled;
      }

      incident_labels = std::move(edge_data.incident_labels);
      location = std::move(edge_data.location);
      small_edge = edge_data.small_edge;
      sample_size = edge_data.sample_size;
      label_count_map = std::move(edge_data.label_count_map);

      // "steal" the sampled array
      sampled = edge_data.sampled;
      edge_data.sampled = nullptr;

      return *this;
    }

    // delete copy and move constructors
    EdgeData(const EdgeData &e) = delete;
    EdgeData(const EdgeData &&e) = delete;
  };
}

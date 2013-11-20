#ifndef HYPERGRAPH_H_
#define HYPERGRAPH_H_

#include <vector>
#include <limits>
#include <algorithm>

#include "../definitions.h"
#include "../macros.h"

namespace hgr {

#define forall_incident_hyperedges(he,hn) \
  for (HyperNodesSizeType i = hypernodes_[hn].begin(), \
                        end = hypernodes_[hn].begin() + hypernodes_[hn].size(); i < end; ++i) { \
  HyperEdgeID he = edges_[i];

#define forall_pins(hn,he) \
  for (HyperEdgesSizeType j = hyperedges_[he].begin(), \
                        end = hyperedges_[he].begin() + hyperedges_[he].size(); j < end; ++j) { \
  HyperNodeID hn = edges_[j];

#define endfor }

template <typename _HyperNodeType, typename _HyperEdgeType,
          typename _HyperNodeWeightType, typename _HyperEdgeWeightType>
class Hypergraph{
 public:
  typedef _HyperNodeType HyperNodeID;
  typedef _HyperEdgeType HyperEdgeID;
  typedef _HyperNodeWeightType HyperNodeWeight;
  typedef _HyperEdgeWeightType HyperEdgeWeight;
  
  Hypergraph(HyperNodeID num_hypernodes, HyperEdgeID num_hyperedges,
             const hMetisHyperEdgeIndexVector& index_vector,
             const hMetisHyperEdgeVector& edge_vector) :
      num_hypernodes_(num_hypernodes),
      num_hyperedges_(num_hyperedges),
      num_pins_(edge_vector.size()),
      current_num_hypernodes_(num_hypernodes_),
      current_num_hyperedges_(num_hyperedges_),
      current_num_pins_(num_pins_),
      hypernodes_(num_hypernodes_, HyperNode(0,0,1)),
      hyperedges_(num_hyperedges_, HyperEdge(0,0,1)),
      edges_(2 * num_pins_,0) {

    VertexID edge_vector_index = 0;
    for (HyperEdgeID i = 0; i < num_hyperedges_; ++i) {
      hyperedges_[i].set_begin(edge_vector_index);
      for (VertexID pin_index = index_vector[i]; pin_index < index_vector[i + 1]; ++pin_index) {
        hyperedges_[i].increase_size();
        edges_[pin_index] = edge_vector[pin_index];
        hypernodes_[edge_vector[pin_index]].increase_size();
        ++edge_vector_index;
      }
    }

    hypernodes_[0].set_begin(num_pins_);
    for (HyperNodeID i = 0; i < num_hypernodes_; ++i) {
      hypernodes_[i + 1].set_begin(hypernodes_[i].begin() + hypernodes_[i].size());
      hypernodes_[i].set_size(0);
    }

    for (HyperEdgeID i = 0; i < num_hyperedges_; ++i) {
      for (VertexID pin_index = index_vector[i]; pin_index < index_vector[i + 1]; ++pin_index) {
        HyperNodeID pin = edge_vector[pin_index];
        edges_[hypernodes_[pin].begin() + hypernodes_[pin].size()] = i;
        hypernodes_[pin].increase_size();
      }
    }    
  }

  // ToDo: add a "pretty print" function...
  void DEBUG_print() {
    for (HyperEdgeID i = 0; i < num_hyperedges_; ++i) {
          PRINT("hyperedge " << i << ": begin=" << hyperedges_[i].begin() << " size="
                << hyperedges_[i].size() << " weight=" << hyperedges_[i].weight());
    }
    for (HyperNodeID i = 0; i < num_hypernodes_; ++i) {
      PRINT("hypernode " << i << ": begin=" << hypernodes_[i].begin() << " size="
            << hypernodes_[i].size()  << " weight=" << hypernodes_[i].weight());
    }
    for (VertexID i = 0; i < edges_.size(); ++i) {
      PRINT("edges_[" << i <<"]=" << edges_[i]);
    }
  }

  // ToDo: This method should return a memento to reconstruct the changes!
  void Contract(HyperNodeID hn_handle_u, HyperNodeID hn_handle_v) {
    ASSERT(!hypernodes_[hn_handle_u].isInvalid() && !hypernodes_[hn_handle_v].isInvalid(),
           "Hypernode is invalid!");
    HyperNode &u = hypernode(hn_handle_u);
    HyperNode &v = hypernode(hn_handle_v);

    u.set_weight(u.weight() + v.weight());
    
    EdgeIterator edge_to_u;
    EdgeIterator w_end;
    EdgeIterator he_it_begin, he_it_end;
    std::tie(he_it_begin, he_it_end) = HyperEdges(hn_handle_v);
    for (EdgeIterator he_it = he_it_begin; he_it != he_it_end; ++he_it) {
      PRINT("Analyzing HE " << *he_it);

      EdgeIterator pin_it_begin, pin_it_end;
      std::tie(pin_it_begin, pin_it_end) = Pins(*he_it);
      ASSERT(pin_it_begin != pin_it_end, "Hyperedge " << *he_it << " is empty");
      --pin_it_end;
      edge_to_u = w_end = pin_it_end;
      for (EdgeIterator pin_it = pin_it_begin; pin_it != pin_it_end; ++pin_it) {
        if (*pin_it == hn_handle_v) {
          PRINT("swapped");
          std::swap(*pin_it, *w_end);
          --pin_it;
        } else if (*pin_it == hn_handle_u) {
          edge_to_u = pin_it;
          PRINT("Found " << hn_handle_u << " in Hyperedge " << *he_it << " on Pos "
                << std::distance(edges_.begin(), pin_it));
        }
      }
      ASSERT(*w_end == hn_handle_v, "v is not last entry in adjacency array!");
      if (edge_to_u != w_end) {
        // u and v are contained in the same hyperedge. therefore we don't need to update u and
        // can just cut off the last entry!
        hyperedges_[*he_it].decrease_size();
      } else {
        // otherwise we have to add a new edge between hypernode vertex u and hyperedge vertex w
        // we reuse the slot of v in w, to make the corresponding connection from hyperedge PoV
        // and add a new edge (u,w) to establish the connection from hypernode PoV
        *w_end = hn_handle_u;
        AddEdge(hn_handle_u, *he_it);
      }
    }
    ClearVertex(hn_handle_v, hypernodes_);
    RemoveVertex(hn_handle_v, hypernodes_);    
}

  void Disconnect(HyperNodeID hn_handle, HyperEdgeID he_handle) {
    ASSERT(!hypernodes_[hn_handle].isInvalid(),"Hypernode is invalid!");
    ASSERT(!hyperedges_[he_handle].isInvalid(),"Hyperedge is invalid!");
    ASSERT(std::count(edges_.begin() + hypernode(hn_handle).begin(),
                      edges_.begin() + hypernode(hn_handle).begin() +
                      hypernode(hn_handle).size(), he_handle) == 1,
           "Hypernode not connected to hyperedge");
    ASSERT(std::count(edges_.begin() + hyperedge(he_handle).begin(),
                      edges_.begin() + hyperedge(he_handle).begin() +
                      hyperedge(he_handle).size(), hn_handle) == 1,
           "Hyperedge does not contain hypernode");
    RemoveEdge(hn_handle, he_handle, hypernodes_);
    RemoveEdge(he_handle, hn_handle, hyperedges_);
  }
  
  void RemoveHyperNode(HyperNodeID hn_handle) {
    ASSERT(!hypernode(hn_handle).isInvalid(),"Hypernode is invalid!");
    forall_incident_hyperedges(he_handle, hn_handle) {
      RemoveEdge(he_handle, hn_handle, hyperedges_);
      --current_num_pins_;
    } endfor
    ClearVertex(hn_handle, hypernodes_);
    RemoveVertex(hn_handle, hypernodes_);
    --current_num_hypernodes_;
  }
  
  void RemoveHyperEdge(HyperEdgeID he_handle) {
    ASSERT(!hyperedge(he_handle).isInvalid(),"Hyperedge is invalid!");
    forall_pins(hn_handle, he_handle) {
      RemoveEdge(hn_handle, he_handle, hypernodes_);
      --current_num_pins_;
    } endfor
    ClearVertex(he_handle, hyperedges_);
    RemoveVertex(he_handle, hyperedges_);
    --current_num_hyperedges_;
  }

  /* ToDo: Design operations a la GetAdjacentHypernodes GetIncidentHyperedges */

  /* Accessors and mutators */
  inline HyperEdgeID hypernode_degree(HyperNodeID hn_handle) const {
    ASSERT(!hypernode(hn_handle).isInvalid(), "Invalid HypernodeID");    
    return hypernode(hn_handle).size();
  }
  
  inline HyperNodeID hyperedge_size(HyperEdgeID he_handle) const {
    ASSERT(!hyperedge(he_handle).isInvalid(), "Invalid HyperedgeID");
    return hyperedge(he_handle).size();
  }

  inline HyperNodeWeight hypernode_weight(HyperNodeID hn_handle) const {
    ASSERT(!hypernode(hn_handle).isInvalid(), "Invalid HypernodeID");
    return hypernode(hn_handle).weight();
  } 

  inline void set_hypernode_weight(HyperNodeID hn_handle,
                                   HyperNodeWeight weight) {
    ASSERT(!hypernode(hn_handle).isInvalid(), "Invalid HypernodeID");
    hypernode(hn_handle).set_weight(weight);
  }
  
  inline HyperEdgeWeight hyperedge_weight(HyperEdgeID he_handle) const {
    ASSERT(!hyperedge(he_handle).isInvalid(), "Invalid HyperedgeID");
    return hyperedge(he_handle).weight();
  }
  
  inline void set_hyperedge_weight(HyperEdgeID he_handle,
                                   HyperEdgeWeight weight) {
    ASSERT(!hyperedge(he_handle).isInvalid(), "Invalid HyperedgeID");
    hyperedge(he_handle).set_weight(weight);
  }
  
  inline HyperNodeID number_of_hypernodes() const {
    return current_num_hypernodes_;
  }

  inline HyperEdgeID number_of_hyperedges() const {
    return current_num_hyperedges_;
  }

  inline HyperNodeID number_of_pins() const {
    return current_num_pins_;
  }
  
 private:
  FRIEND_TEST(HypergraphTest, RemovesHypernodes);
  FRIEND_TEST(HypergraphTest, InitializesInternalHypergraphRepresentation);
  FRIEND_TEST(HypergraphTest, DisconnectsHypernodeFromHyperedge);
  
  typedef unsigned int VertexID;
  
  template <typename VertexTypeTraits>
  class InternalVertex {
   public:
    typedef typename VertexTypeTraits::WeightType WeightType;

    InternalVertex(VertexID begin, VertexID size,
                   WeightType weight) :
        begin_(begin),
        size_(size),
        weight_(weight) {}

    InternalVertex() :
        begin_(0),
        size_(0),
        weight_(0) {}
    
    inline void Invalidate() {
      ASSERT(!isInvalid(), "Vertex is already invalidated");
      begin_ = std::numeric_limits<VertexID>::max();
    }

    inline bool isInvalid() const {
      return begin_ == std::numeric_limits<VertexID>::max();
    }

    inline VertexID begin() const { return begin_; }
    inline void set_begin(VertexID begin) { begin_ = begin; }

    inline VertexID size() const { return size_; }
    inline void set_size(VertexID size) { size_ = size; }
    inline void increase_size() { ++size_; }
    inline void decrease_size() {
      ASSERT(size_ > 0, "Size out of bounds");
      --size_;
      if (size_ == 0) { Invalidate(); }
    }
    
    inline WeightType weight() const { return weight_; }
    inline void set_weight(WeightType weight) { weight_ = weight; }
    
   private:
    VertexID begin_;
    VertexID size_;
    WeightType weight_;
  };

  struct HyperNodeTraits {
    typedef HyperNodeWeight WeightType;    
  };
    
  struct HyperEdgeTraits {
    typedef HyperNodeWeight WeightType;    
  };

  typedef InternalVertex<HyperNodeTraits> HyperNode;
  typedef InternalVertex<HyperEdgeTraits> HyperEdge;
  typedef typename std::vector<HyperNode>::iterator HyperNodeIterator; // ToDo: make own iterator
  typedef typename std::vector<HyperEdge>::iterator HyperEdgeIterator; // ToDo: make own iterator
  typedef typename std::vector<VertexID>::iterator EdgeIterator;
  typedef typename std::vector<HyperNode>::size_type HyperNodesSizeType;
  typedef typename std::vector<HyperEdge>::size_type HyperEdgesSizeType;
  typedef typename std::vector<VertexID>::size_type DegreeSizeType;

  template <typename T>
  inline void ClearVertex(VertexID vertex, T& container) {
    ASSERT(vertex < container.size(), "VertexID out of bounds");
    container[vertex].set_size(0);
  }

  template <typename T>
  inline void RemoveVertex(VertexID vertex, T& container) {
    ASSERT(vertex < container.size(), "VertexID out of bounds");
    ASSERT(container[vertex].size() == 0, "Vertex is not cleared");
    container[vertex].Invalidate();
  }

  // Current version copies all previous entries to ensure consistency. We might change
  // this to only add the corresponding entry and let the caller handle the consistency issues!
  void AddEdge(HyperNodeID hn_handle, HyperEdgeID he_handle) {
    // TODO: Assert via TypeInfo that we create an edge from Hypernode to Hyperedge
    ASSERT(!hypernode(hn_handle).isInvalid(), "Invalid HypernodeID");
    ASSERT(!hyperedge(he_handle).isInvalid(), "Invalid HyperedgeID");
    
    HyperNode &hn = hypernode(hn_handle);
    if (hn.begin() + hn.size() != edges_.size()) {
      edges_.insert(edges_.end(), edges_.begin() + hn.begin(),
                    edges_.begin() + hn.begin() + hn.size());
      hn.set_begin(edges_.size() - hn.size());
    }
    ASSERT(hn.begin() + hn.size() == edges_.size(), "AddEdge inconsistency");
    edges_.push_back(he_handle);
    hn.increase_size();
  }

  template <typename Handle1, typename Handle2, typename Container >
  inline void RemoveEdge(Handle1 u, Handle2 v, Container& container) {
   typename Container::reference &vertex = container[u];
    ASSERT(!vertex.isInvalid(), "InternalVertex is invalid");
    
    EdgeIterator begin = edges_.begin() + vertex.begin();
    ASSERT(vertex.size() > 0, "InternalVertex is empty!");
    EdgeIterator last_entry =  begin + vertex.size() - 1;
    while (*begin != v) {
      ++begin;
    }
    std::swap(*begin, *last_entry);
    vertex.decrease_size();    
  }

  inline std::pair<EdgeIterator, EdgeIterator> HyperEdges(HyperNodeID v) {
    return std::make_pair(edges_.begin() + hypernodes_[v].begin(),
                          edges_.begin() + hypernodes_[v].begin() + hypernodes_[v].size());
  }

  inline std::pair<EdgeIterator, EdgeIterator> Pins(HyperEdgeID v) {
    return std::make_pair(edges_.begin() + hyperedges_[v].begin(),
                          edges_.begin() + hyperedges_[v].begin() + hyperedges_[v].size());
  }

  inline const HyperNode& hypernode(HyperNodeID id) const{
    ASSERT(id < num_hypernodes_, "Hypernode does not exist");
    return hypernodes_[id];
  }

  inline const HyperEdge& hyperedge(HyperEdgeID id) const {
    ASSERT(id < num_hyperedges_, "Hyperedge does not exist");
    return hyperedges_[id];
  }
 
  // To avoid code duplication we implement non-const version in terms of const version
  inline HyperNode& hypernode(HyperNodeID id) {
    return const_cast<HyperNode&>(static_cast<const Hypergraph&>(*this).hypernode(id));
  }

  inline HyperEdge& hyperedge(HyperEdgeID id) {
    return const_cast<HyperEdge&>(static_cast<const Hypergraph&>(*this).hyperedge(id));
  }

  const HyperNodeID num_hypernodes_;
  const HyperEdgeID num_hyperedges_;
  const HyperNodeID num_pins_;

  HyperNodeID current_num_hypernodes_;
  HyperEdgeID current_num_hyperedges_;
  HyperNodeID current_num_pins_;
  
  std::vector<HyperNode> hypernodes_;
  std::vector<HyperEdge> hyperedges_;
  std::vector<VertexID> edges_; 

  DISALLOW_COPY_AND_ASSIGN(Hypergraph);
};

} // namespace hgr
#endif  // HYPERGRAPH_H_

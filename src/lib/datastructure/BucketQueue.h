/******************************************************************************
 * bucket_pq.h 
 *
 * Source of KaHIP -- Karlsruhe High Quality Partitioning.
 *
 * Modified to suite KaHyPar 
 *
 ******************************************************************************
 * Copyright (C) 2013 Christian Schulz <christian.schulz@kit.edu>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *****************************************************************************/

#ifndef BUCKET_PQ_EM8YJPA9
#define BUCKET_PQ_EM8YJPA9

#include <limits>
#include <unordered_map>

#include "lib/macros.h"

namespace datastructure {

template <typename NodeID, typename Gain, typename EdgeWeight>
class BucketPQ {
 public:
  BucketPQ( const EdgeWeight & gain_span ) :
      m_elements(0),
      m_gain_span(gain_span),
      m_max_idx(0),
      m_queue_index(),
      m_buckets() {
    m_buckets.resize(2*m_gain_span+1);
  }

  virtual ~BucketPQ() {};

  NodeID size() {
    return m_elements;  
  }

  Gain key(NodeID element) {
    ASSERT(m_queue_index.find(element) != m_queue_index.end(),
           " Element " << element << " not contained in PQ");
    return m_queue_index[element].second;        
  }

  void insert(NodeID node, Gain gain) {
    reInsert(node,gain);
  }
  
  void reInsert(NodeID node, Gain gain) {
    unsigned address = gain + m_gain_span;
    if(address > m_max_idx) {
      m_max_idx = address; 
    }
    
    m_buckets[address].push_back( node ); 
    m_queue_index[node].first  = m_buckets[address].size() - 1; //store position
    m_queue_index[node].second = gain;

    m_elements++;

  }
  
  bool empty() {
    return m_elements == 0;
  }

  void clear() {
    for (auto& bucket : m_buckets) {
      bucket.clear();
    }
    m_elements  = 0;
    m_max_idx   = 0;
    m_queue_index.clear();
  }

  Gain maxKey() {
    ASSERT(!empty(), "BucketPQ is empty");
    //   DBG(true, "---->" << m_queue_index[m_buckets[m_max_idx].back()].second);
    return m_max_idx - m_gain_span;        
  }
  
  NodeID max() {
    ASSERT(!m_buckets[m_max_idx].empty(),
           "max-Bucket " << m_max_idx << " is empty");
    return m_buckets[m_max_idx].back();        
  }
  
  NodeID deleteMax() {
    ASSERT(!m_buckets[m_max_idx].empty(),
           "max-Bucket " << m_max_idx << " is empty");
    NodeID node = m_buckets[m_max_idx].back();
    m_buckets[m_max_idx].pop_back();
    m_queue_index.erase(node);
    
    if( m_buckets[m_max_idx].size() == 0 ) {
      //update max_idx
      while( m_max_idx != 0 )  {
        m_max_idx--;
        if(m_buckets[m_max_idx].size() > 0) {
          break;
        }
      }
    }

    m_elements--;
    return node;        
  }

  void decreaseKey(NodeID node, Gain newGain) {
    updateKey( node, newGain );
  }
  void increaseKey(NodeID node, Gain newGain) {
    updateKey( node, newGain );
  }

  void updateKey(NodeID node, Gain new_gain) {
    remove(node);
    reInsert(node, new_gain);
  }

  void remove(NodeID node) {
    ASSERT(m_queue_index.find(node) != m_queue_index.end(),
           "Hypernode " << node << " not in PQ");
    unsigned int in_bucket_idx = m_queue_index[node].first;
    Gain  old_gain      = m_queue_index[node].second;
    unsigned address    = old_gain + m_gain_span;

    if( m_buckets[address].size() > 1 ) {
      //swap current element with last element and pop_back
      m_queue_index[m_buckets[address].back()].first = in_bucket_idx; // update helper structure
      std::swap(m_buckets[address][in_bucket_idx], m_buckets[address].back());
      m_buckets[address].pop_back();
    } else {
      //size is 1
      m_buckets[address].pop_back();
      if( address == m_max_idx ) {
        //update max_idx
        while( m_max_idx != 0 )  {
          m_max_idx--;
          if(m_buckets[m_max_idx].size() > 0) {
            break;
          }
        }

      }
    }

    m_elements--;
    m_queue_index.erase(node);
  }

  bool contains(NodeID node) {
    return m_queue_index.find(node) != m_queue_index.end(); 
  }
 private:
  NodeID     m_elements;
  EdgeWeight m_gain_span;
  unsigned   m_max_idx; //points to the non-empty bucket with the largest gain
                
  std::unordered_map<NodeID, std::pair<unsigned int, Gain> > m_queue_index;
  std::vector< std::vector<NodeID> >  m_buckets;
};


} // namespace datastructure
#endif /* end of include guard: BUCKET_PQ_EM8YJPA9 */

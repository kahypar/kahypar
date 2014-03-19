/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_REFINEMENT_HYPEREDGEFMQUEUECLOGGINGPOLICIES_H_
#define SRC_PARTITION_REFINEMENT_HYPEREDGEFMQUEUECLOGGINGPOLICIES_H_

namespace partition {
static const bool dbg_refinement_queue_clogging = false;

struct OnlyRemoveIfBothQueuesClogged {
  template <typename Queue>
  static bool removeCloggingQueueEntries(bool pq0_eligible, bool pq1_eligible,
                                         Queue& pq0, Queue& pq1,
                                         boost::dynamic_bitset<uint64_t>& indicator) {
    if (!pq0_eligible && !pq1_eligible) {
      if (!pq0->empty()) {
        DBG(dbg_refinement_queue_clogging, " Removing HE/HN " << pq0->max() << " from PQ 0");
        indicator[pq0->max()] = 1;
        pq0->deleteMax();
      }
      if (!pq1->empty()) {
        DBG(dbg_refinement_queue_clogging, " Removing HE/HN " << pq1->max() << " from PQ 1");
        indicator[pq1->max()] = 1;
        pq1->deleteMax();
      }
      return true;
    }
    return false;
  }

  template <typename Queue>
  static bool removeCloggingQueueEntries(bool pq0_eligible, bool pq1_eligible,
                                         Queue& pq0, Queue& pq1) {
    if (!pq0_eligible && !pq1_eligible) {
      if (!pq0->empty()) {
        DBG(dbg_refinement_queue_clogging, " Removing HE/HN " << pq0->max() << " from PQ 0");
        pq0->deleteMax();
      }
      if (!pq1->empty()) {
        DBG(dbg_refinement_queue_clogging, " Removing HE/HN " << pq1->max() << " from PQ 1");
        pq1->deleteMax();
      }
      return true;
    }
    return false;
  }

  protected:
  ~OnlyRemoveIfBothQueuesClogged() { }
};

struct RemoveOnlyTheCloggingEntry {
  template <typename Queue>
  static bool removeCloggingQueueEntries(bool pq0_eligible, bool pq1_eligible,
                                         Queue& pq0, Queue& pq1,
                                         boost::dynamic_bitset<uint64_t>& indicator) {
    bool removed_a_node = false;
    if (!pq0_eligible && !pq0->empty()) {
      DBG(dbg_refinement_queue_clogging, " Removing HE/HN " << pq0->max() << " from PQ 0");
      indicator[pq0->max()] = 1;
      pq0->deleteMax();
      removed_a_node =  true;
    }
    if (!pq1_eligible && !pq1->empty()) {
      DBG(dbg_refinement_queue_clogging, " Removing HE/HN " << pq1->max() << " from PQ 1");
      indicator[pq1->max()] = 1;
      pq1->deleteMax();
      removed_a_node =  true;
    }
    return removed_a_node;
  }
  template <typename Queue>
  static bool removeCloggingQueueEntries(bool pq0_eligible, bool pq1_eligible,
                                         Queue& pq0, Queue& pq1) {
    bool removed_a_node = false;
    if (!pq0_eligible && !pq0->empty()) {
      DBG(dbg_refinement_queue_clogging, " Removing HE/HN " << pq0->max() << " from PQ 0");
      pq0->deleteMax();
      removed_a_node =  true;
    }
    if (!pq1_eligible && !pq1->empty()) {
      DBG(dbg_refinement_queue_clogging, " Removing HE/HN " << pq1->max() << " from PQ 1");
      pq1->deleteMax();
      removed_a_node =  true;
    }
    return removed_a_node;
  }

  protected:
  ~RemoveOnlyTheCloggingEntry() { }
};
} // namespace partition

#endif  // SRC_PARTITION_REFINEMENT_HYPEREDGEFMQUEUECLOGGINGPOLICIES_H_

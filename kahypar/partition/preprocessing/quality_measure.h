/***************************************************************************
 *  Copyright (C) 2015 Tobias Heuer <tobias.heuer@gmx.net>
 *  Copyright (C) 2015-2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#pragma once

#include <algorithm>
#include <limits>
#include <vector>

#include "kahypar/macros.h"
#include "kahypar/definitions.h"
#include "kahypar/datastructure/graph.h"
#include "kahypar/datastructure/fast_reset_flag_array.h"

namespace kahypar {

using ds::Edge;
using ds::FastResetFlagArray;
#define EPS 1e-5

const bool dbg_modularity_function = false;

class QualityMeasure {

friend class Modularity;
    
public:
    
    QualityMeasure(Graph& graph, const Configuration& config) : graph(graph), config(config) { }
    
    virtual ~QualityMeasure() { }
    
    virtual void remove(NodeID node, EdgeWeight incidentCommWeight)=0;
    virtual void insert(NodeID node, ClusterID new_cid, EdgeWeight incidentCommWeight)=0;
    virtual EdgeWeight gain(NodeID node, ClusterID cid, EdgeWeight incidentCommWeight)=0;
    virtual EdgeWeight quality()=0;

private:
    Graph& graph;
    const Configuration& config;
    
};

class Modularity : public QualityMeasure {

public:
    Modularity(Graph& graph, const Configuration& config) : QualityMeasure(graph,config), in(graph.numNodes(),0), tot(graph.numNodes(),0), vis(graph.numNodes()) { 
        for(NodeID node : graph.nodes()) {
            ClusterID cur_cid = graph.clusterID(node);
            for(auto cluster : graph.incidentClusterWeightOfNode(node)) {
                ClusterID cid = cluster.clusterID;
                EdgeWeight weight = cluster.weight;
                if(cid == cur_cid) {
                    in[cur_cid] += weight;
                    break;
                }
            }
            tot[cur_cid] += graph.weightedDegree(node);
        }
    }
    
    ~Modularity() {
        in.clear();
        tot.clear();
    }
    
    inline void remove(NodeID node, EdgeWeight incidentCommWeight) {
        ASSERT(node < graph.numNodes(), "NodeID " << node << " doesn't exist!");
        ClusterID cid = graph.clusterID(node);
        
        in[cid] -= 2.0L*incidentCommWeight + graph.selfloopWeight(node);
        tot[cid] -= graph.weightedDegree(node);
        
        graph.setClusterID(node,-1);
    }
    
    inline void insert(NodeID node, ClusterID new_cid, EdgeWeight incidentCommWeight) {
        ASSERT(node < graph.numNodes(), "NodeID " << node << " doesn't exist!");
        ASSERT(graph.clusterID(node) == -1, "Node " << node << " isn't a isolated node!");
        
        in[new_cid] += 2.0L*incidentCommWeight + graph.selfloopWeight(node);
        tot[new_cid] += graph.weightedDegree(node);
        
        graph.setClusterID(node,new_cid);
        
        ASSERT([&]() {
           if(!dbg_modularity_function) return true;
           EdgeWeight q = quality();
           return q < std::numeric_limits<EdgeWeight>::max();
        }(), "");
    }
    
    inline EdgeWeight gain(NodeID node, ClusterID cid, EdgeWeight incidentCommWeight) {
        ASSERT(node < graph.numNodes(), "NodeID " << node << " doesn't exist!");
        ASSERT(graph.clusterID(node) == -1, "Node " << node << " isn't a isolated node!");
        
        EdgeWeight totc = tot[cid];
        EdgeWeight m2 = graph.totalWeight();
        EdgeWeight w_degree = graph.weightedDegree(node);
        
        EdgeWeight gain = incidentCommWeight - totc*w_degree/m2;
        
        ASSERT([&]() {
            if(!dbg_modularity_function) return true;
            EdgeWeight modularity_before = modularity();
            insert(node,cid,incidentCommWeight);
            EdgeWeight modularity_after = modularity();
            remove(node,incidentCommWeight);
            EdgeWeight real_gain = modularity_after - modularity_before;
            if(std::abs(2.0L*gain/m2 - real_gain) > EPS) {
                LOGVAR(real_gain);
                LOGVAR(2.0L*gain/m2);
                return false;
            }
            return true;
        }(), "Gain calculation failed!");
        
        return gain;
    }
    
    
    EdgeWeight quality() {
        EdgeWeight q = 0.0L;
        EdgeWeight m2 = graph.totalWeight();
        for(NodeID node : graph.nodes()) {
            if(tot[node] > EPS) {
                q += in[node] - (tot[node]*tot[node])/m2;
            }
        }
        
        q /= m2;
        
        ASSERT(!dbg_modularity_function || std::abs(q-modularity()) < EPS, "Calculated modularity (q=" << q << ") is not equal with the real modularity (modularity=" << modularity() << ")!");
        
        return q;
    }
    
private:
    FRIEND_TEST(AModularityMeasure,IsCorrectInitialized);    
    FRIEND_TEST(AModularityMeasure,RemoveNodeFromCommunity);
    FRIEND_TEST(AModularityMeasure,InsertNodeInCommunity);
    FRIEND_TEST(AModularityMeasure,RemoveNodeFromCommunityWithMoreThanOneNode);
    
    EdgeWeight modularity() {
        EdgeWeight q = 0.0L;
        EdgeWeight m2 = graph.totalWeight();
        
        for(NodeID u : graph.nodes()) {
            for(Edge edge : graph.adjacentNodes(u)) {
                NodeID v = edge.targetNode;
                vis.set(v,true);
                if(graph.clusterID(u) == graph.clusterID(v)) {
                    q += edge.weight - (graph.weightedDegree(u)*graph.weightedDegree(v))/m2;
                }
            }
            for(NodeID v : graph.nodes()) {
                if(graph.clusterID(u) == graph.clusterID(v) && !vis[v]) {
                    q -= (graph.weightedDegree(u)*graph.weightedDegree(v))/m2;
                }
            }
            vis.reset();
        }
        
        q /= m2;
        return q;
    }
    
    using QualityMeasure::graph;
    using QualityMeasure::config;
    std::vector<EdgeWeight> in;
    std::vector<EdgeWeight> tot;
    FastResetFlagArray<> vis;
    
    
};

}  // namespace kahypar


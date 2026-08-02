#pragma once

#include <vector>
#include "HNSWNode.h"

class HNSW
{
private:
 // Stores all nodes in the graph.
   std::vector<HNSWNode*> nodes;
//   to connect the node to eachother
   void connect(HNSWNode* node1, HNSWNode* node2);

//    to find the nearest neighbor of a given node
   HNSWNode* findNearestNeighbor(HNSWNode* newnode,int k);
public:
//  creating graph
  void insert(const VectorRecord& record);
};
#include "HNSW.h"

void HNSW::insert(const VectorRecord& record)
{
    // Create a new node.
    HNSWNode* newNode = new HNSWNode(record);

    // Add it to the graph.
    nodes.push_back(newNode);
}

// connection neighbor
void HNSW::connect(HNSWNode* node1, HNSWNode* node2)
{
    // Connect node1 to node2 and vice versa.
    node1->neighbors.push_back(node2);
    node2->neighbors.push_back(node1);
}
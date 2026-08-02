#pragma once

#include "KDNode.h"

class KDTree
{
private:
    // Pointer to the root of the tree.
    KDNode *root;

    KDNode *insert(
        KDNode *node,
        const VectorRecord &record,
        int depth);
    void display(KDNode *node) const;

    void nearestNeighbor(
        KDNode *node,
        const std::vector<float> &query,
        int depth,
        KDNode *&bestNode,
        float &bestDistance);

public:
    // Constructor
    KDTree();

    // Insert a new record into the KD-Tree.
    void insert(const VectorRecord &record);
    void display() const;

    VectorRecord nearestNeighbor(
        const std::vector<float> &query);
};
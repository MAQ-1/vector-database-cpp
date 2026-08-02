#pragma once
#include "VectorRecord.h"

class KDNode{
    public:

    // stores the vector and its metadata
    VectorRecord record;
        // Left child.
    KDNode* left;
    // Right child.
    KDNode* right;

    KDNode(const VectorRecord& record);
};
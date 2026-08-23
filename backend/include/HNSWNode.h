#pragma once

#include <vector>
#include "VectorRecord.h"

struct HNSWNode
{
    VectorRecord record;
    int level;

    std::vector<std::vector<HNSWNode*>> neighbors;

    HNSWNode(const VectorRecord& record, int level)
        : record(record), level(level)
    {
         // Create one neighbor list for every level.
    neighbors.resize(level + 1);
    }

    
};
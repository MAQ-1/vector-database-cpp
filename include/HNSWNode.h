#pragma once

#include <vector>
#include "VectorRecord.h"

struct HNSWNode
{
    VectorRecord record;

    std::vector<HNSWNode*> neighbors;

    HNSWNode(const VectorRecord& record)
        : record(record)
    {
    }
};
#pragma once

#include "VectorRecord.h"

struct SearchResult
{
    VectorRecord record;
    float score;
};

struct CompareSearchResult
{
    bool operator()(const SearchResult& a,
                    const SearchResult& b) const
    {
        return a.score > b.score;
    }
};
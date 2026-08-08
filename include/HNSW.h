#pragma once
#include <queue>
#include <utility>
#include <vector>
#include "HNSWNode.h"
#include <unordered_set>


class HNSW
{
    using Candidate = std::pair<float, HNSWNode*>;
     static constexpr int M = 4;
private:
    std::vector<HNSWNode*> nodes;

    HNSWNode* entryPoint;

    void connect(HNSWNode* node1, HNSWNode* node2);

    std::vector<HNSWNode*> findNearestNeighbors(
        HNSWNode* newNode,
        int k);

    std::vector<HNSWNode*> efSearch(
        HNSWNode* startNode,
        const std::vector<float>& query,
        int level,
        int ef);

    int generateRandomLevel();

    void pruneNeighbors(HNSWNode* node, int level);

public:
    HNSW();

    void insert(const VectorRecord& record);
    HNSWNode* getEntryPoint() const;
    HNSWNode* greedySearch(
        HNSWNode* startNode,
        const std::vector<float>& query,
        int level);

        VectorRecord search(
    const std::vector<float>& query
);

void remove(int id);

    int nodeCount() const;

};
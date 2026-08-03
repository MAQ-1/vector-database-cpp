#include "HNSW.h"
#include "Similarity.h"
#include <algorithm>
#include <cstdlib> // for rand()
#include "HNSWNode.h"
HNSW::HNSW()
{
    entryPoint = nullptr;
}
void HNSW::insert(const VectorRecord &record)
{
    // Generate a random level for the new node.
    int level = generateRandomLevel();

    // Create the new node.
    HNSWNode *newNode = new HNSWNode(record, level);

    // First node in the graph.
    if (nodes.empty())
    {
        nodes.push_back(newNode);
        entryPoint = newNode;
        return;
    }

    // Start from the current entry point.
    HNSWNode *current = entryPoint;

    // Search from the highest level down to the new node's level.
    for (int currentLevel = entryPoint->level;
         currentLevel >= newNode->level;
         currentLevel--)
    {
        current = greedySearch(
            current,
            newNode->record.embedding,
            currentLevel);
    }

    // Number of candidates to explore.
    const int efConstruction = 10;

    // Find the best candidate neighbors.
    std::vector<HNSWNode *> nearestNeighbors =
        efSearch(
            current,
            newNode->record.embedding,
            0,
            efConstruction);

    // Connect the new node to every selected neighbor.
    for (HNSWNode *neighbor : nearestNeighbors)
    {
        connect(newNode, neighbor);
    }

    // Add the new node to the graph.
    nodes.push_back(newNode);

    // Update the entry point if necessary.
    if (newNode->level > entryPoint->level)
    {
        entryPoint = newNode;
    }
}

// Greedy Search
// Finds a nearby node by moving to better neighbors.
// Greedy Search on a specific level.
HNSWNode *HNSW::greedySearch(
    HNSWNode *startNode,
    const std::vector<float> &query,
    int level)
{
    // Start from the given node.
    HNSWNode *current = startNode;

    // Distance from current node to query.
    float currentDistance =
        Similarity::euclideanDistance(
            current->record.embedding,
            query);

    // Keep moving while we find agreedySearch better neighbor.
    while (true)
    {
        HNSWNode *bestNode = current;
        float bestDistance = currentDistance;

        // Check only the neighbors on this level.
        for (HNSWNode *neighbor : current->neighbors[level])
        {
            float distance =
                Similarity::euclideanDistance(
                    neighbor->record.embedding,
                    query);

            if (distance < bestDistance)
            {
                bestDistance = distance;
                bestNode = neighbor;
            }
        }

        // No better neighbor found.
        if (bestNode == current)
        {
            break;
        }

        // Move to the better node.
        current = bestNode;
        currentDistance = bestDistance;
    }

    return current;
}

// connection neighbor
void HNSW::connect(HNSWNode *node1, HNSWNode *node2)
{
    int minLevel = std::min(node1->level, node2->level);

    for (int level = 0; level <= minLevel; level++)
    {
        bool alreadyConnected = false;

        // Check whether node2 is already a neighbor of node1.
        for (HNSWNode *neighbor : node1->neighbors[level])
        {
            if (neighbor == node2)
            {
                alreadyConnected = true;
                break;
            }
        }

        // If they are already connected, skip this level.
        if (alreadyConnected)
        {
            continue;
        }

        // Otherwise create the connection.
        node1->neighbors[level].push_back(node2);
        node2->neighbors[level].push_back(node1);

        // Keep only the best M neighbors.
        pruneNeighbors(node1, level);
        pruneNeighbors(node2, level);
    }
}

HNSWNode* HNSW::getEntryPoint() const
{
    return entryPoint;
}

void HNSW::pruneNeighbors(HNSWNode* node, int level)
{
    // Already within the limit.
    if (node->neighbors[level].size() <= M)
    {
        return;
    }

    int farthestIndex = 0;

    float farthestDistance =
        Similarity::euclideanDistance(
            node->record.embedding,
            node->neighbors[level][0]->record.embedding);

    // Find the farthest neighbor.
    for (int i = 1; i < node->neighbors[level].size(); i++)
    {
        float distance = Similarity::euclideanDistance(
            node->record.embedding,
            node->neighbors[level][i]->record.embedding);

        if (distance > farthestDistance)
        {
            farthestDistance = distance;
            farthestIndex = i;
        }
    }

    // Remove the farthest neighbor.
    node->neighbors[level].erase(
        node->neighbors[level].begin() + farthestIndex);
}

VectorRecord HNSW::search(const std::vector<float>& query)
{
    // Empty graph.
    if (nodes.empty())
    {
        throw std::runtime_error("HNSW graph is empty.");
    }

    // Step 1: Navigate quickly to the correct region.
    HNSWNode* current = entryPoint;

    for (int level = entryPoint->level; level >= 0; level--)
    {
        current = greedySearch(current, query, level);
    }

    // Step 2: Explore around that region.
    const int efSearchValue = 10;

    std::vector<HNSWNode*> candidates =
        efSearch(current, query, 0, efSearchValue);

    // Step 3: Return the closest candidate.
    if (candidates.empty())
{
    return current->record;
}

return candidates[0]->record;
}

std::vector<HNSWNode *> HNSW::efSearch(
    HNSWNode *startNode,
    const std::vector<float> &query,
    int level,
    int ef)
{
    std::vector<HNSWNode *> result;

    std::priority_queue<
        Candidate,
        std::vector<Candidate>,
        std::greater<Candidate>>
        candidates;

    float distance = Similarity::euclideanDistance(
        startNode->record.embedding,
        query);

    candidates.push({distance, startNode});

    std::unordered_set<HNSWNode *> visited;
    visited.insert(startNode);

    while (!candidates.empty())
    {
        Candidate current = candidates.top();
        candidates.pop();

        HNSWNode *currentNode = current.second;

        result.push_back(currentNode);

        if (result.size() >= ef)
        {
            break;
        }

        for (HNSWNode *neighbor : currentNode->neighbors[level])
        {
            if (visited.find(neighbor) != visited.end())
            {
                continue;
            }

            visited.insert(neighbor);

            float distance = Similarity::euclideanDistance(
                neighbor->record.embedding,
                query);

            candidates.push({distance, neighbor});
        }
    }

    return result;
}

std::vector<HNSWNode *> HNSW::findNearestNeighbors(
    HNSWNode *newNode,
    int k)
{
    // Store every node along with its distance.
    std::vector<std::pair<HNSWNode *, float>> distances;

    // Compute the distance from the new node
    // to every existing node.
    for (HNSWNode *node : nodes)
    {
        float distance = Similarity::euclideanDistance(
            node->record.embedding,
            newNode->record.embedding);

        distances.push_back({node, distance});
    }

    // Sort nodes by distance.
    std::sort(
        distances.begin(),
        distances.end(),
        [](const auto &a, const auto &b)
        {
            return a.second < b.second;
        });

    // Store the K nearest nodes.
    std::vector<HNSWNode *> nearest;

    for (int i = 0; i < k && i < distances.size(); i++)
    {
        nearest.push_back(distances[i].first);
    }

    return nearest;
}

//  get a random level for the new node
int HNSW::generateRandomLevel()
{
    int level = 0;
    while (rand() % 2 == 1) // 50% chance to go to the next level
    {
        level++;
    }

    return level;
}
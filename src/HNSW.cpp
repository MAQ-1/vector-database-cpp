#include "HNSW.h"
#include "Similarity.h"
#include <algorithm>
#include <cstdlib> // for rand()
#include "HNSWNode.h"
#include <stdexcept>
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

    // Greedy descend from the top level down to newNode's level + 1.
    // These levels are skipped — we only need the closest entry point.
    for (int currentLevel = entryPoint->level;
         currentLevel > newNode->level;
         currentLevel--)
    {
        current = greedySearch(
            current,
            newNode->record.embedding,
            currentLevel);
    }

    // Number of candidates to explore per level.
    const int efConstruction = 200;

    // From newNode's level down to 0, run efSearch and connect M neighbors.
    // Start at newNode->level (not min(newNode->level, entryPoint->level)).
    // When newNode->level > entryPoint->level, the old min() clipped the
    // loop start to entryPoint->level, silently skipping all levels above
    // it and leaving newNode's upper neighbor lists permanently empty.
    for (int currentLevel = newNode->level;
         currentLevel >= 0;
         currentLevel--)
    {
        // At levels above the current entry point's level, no existing node
        // has neighbors at currentLevel yet. efSearch would access
        // current->neighbors[currentLevel] out of bounds. Instead, connect
        // newNode directly to the entry point at the levels they share
        // (connect() uses min(node1->level, node2->level) internally).
        if (currentLevel > entryPoint->level)
        {
            connect(newNode, entryPoint);
            continue;
        }

        std::vector<HNSWNode *> nearestNeighbors =
            efSearch(current, newNode->record.embedding, currentLevel, efConstruction);

        // Connect up to M neighbors at this level.
        for (int i = 0; i < static_cast<int>(nearestNeighbors.size()) && i < M; i++)
        {
            connect(newNode, nearestNeighbors[i]);
        }

        // Best neighbor becomes the entry point for the next level down.
        if (!nearestNeighbors.empty())
        {
            current = nearestNeighbors[0];
        }
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







HNSWNode *HNSW::getEntryPoint() const
{
    return entryPoint;
}






void HNSW::pruneNeighbors(HNSWNode *node, int level)
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






VectorRecord HNSW::search(const std::vector<float> &query)
{
    // Empty graph.
    if (nodes.empty())
    {
        throw std::runtime_error("HNSW graph is empty.");
    }

    // Step 1: Navigate quickly to the correct region.
    HNSWNode *current = entryPoint;

    for (int level = entryPoint->level; level >= 0; level--)
    {
        current = greedySearch(current, query, level);
    }

    // Step 2: Explore around that region.
    const int efSearchValue = 50;

    std::vector<HNSWNode *> candidates =
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
    // candidates: min-heap on distance — the frontier to expand next.
    // Closest node is always at the top.
    std::priority_queue<
        Candidate,
        std::vector<Candidate>,
        std::greater<Candidate>>
        candidates;

    // results: max-heap on distance — the current best-ef set.
    // Worst (farthest) node is at the top, so we can evict it cheaply
    // when a closer node arrives.
    std::priority_queue<Candidate> results;

    std::unordered_set<HNSWNode *> visited;
    visited.reserve(ef * 2);

    float startDistance = Similarity::euclideanDistance(
        startNode->record.embedding,
        query);

    candidates.push({startDistance, startNode});
    results.push({startDistance, startNode});
    visited.insert(startNode);

    while (!candidates.empty())
    {
        Candidate current = candidates.top();
        candidates.pop();

        // Standard HNSW termination: if the closest unexplored candidate
        // is already farther than the worst result we have, no future
        // expansion can improve the result set.
        if (current.first > results.top().first)
        {
            break;
        }

        for (HNSWNode *neighbor : current.second->neighbors[level])
        {
            if (visited.count(neighbor))
            {
                continue;
            }

            visited.insert(neighbor);

            float dist = Similarity::euclideanDistance(
                neighbor->record.embedding,
                query);

            // Admit the neighbor if the result set is not full yet,
            // or if it is closer than the current worst result.
            if (static_cast<int>(results.size()) < ef ||
                dist < results.top().first)
            {
                candidates.push({dist, neighbor});
                results.push({dist, neighbor});

                // Evict the farthest result if we are over capacity.
                if (static_cast<int>(results.size()) > ef)
                {
                    results.pop();
                }
            }
        }
    }

    // Drain the max-heap into a vector and reverse to get ascending order.
    std::vector<HNSWNode *> result;
    result.reserve(results.size());

    while (!results.empty())
    {
        result.push_back(results.top().second);
        results.pop();
    }

    // Max-heap drains largest-first; reverse for closest-first.
    std::reverse(result.begin(), result.end());

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
    // Maximum level cap prevents unbounded level growth.
    // log2(10000) ~ 13, so 16 is a safe upper bound.
    const int maxLevel = 16;

    int level = 0;
    while (level < maxLevel && rand() % 2 == 1)
    {
        level++;
    }
    return level;
}

void HNSW::remove(int id)
{
    // Step 1: Find the first node whose record.id matches.
    HNSWNode* target = nullptr;
    int targetIndex = -1;

    for (int i = 0; i < static_cast<int>(nodes.size()); i++)
    {
        if (nodes[i]->record.id == id)
        {
            target = nodes[i];
            targetIndex = i;
            break;
        }
    }

    // Step 2: Id not found — silently return, no side effects.
    // Fallback: no error-handling convention for "not found" exists in the
    // attached code (search() throws only on empty graph, not missing id),
    // so silent return is used here.
    if (target == nullptr)
    {
        return;
    }

    // Step 3: Remove target from every neighbor list in the graph.
    // Iterate every remaining node A. For each level that A actually has
    // (0..A->level inclusive), scan A's neighbor list and erase target.
    // We use A->level as the bound — never target->level — because connect()
    // links at levels 0..min(A->level, target->level), so target can appear
    // in any of A's levels up to A's own cap.
    for (HNSWNode* node : nodes)
    {
        if (node == target)
        {
            continue;
        }

        for (int lvl = 0; lvl <= node->level; lvl++)
        {
            std::vector<HNSWNode*>& nbrs = node->neighbors[lvl];
            nbrs.erase(
                std::remove(nbrs.begin(), nbrs.end(), target),
                nbrs.end());
        }
    }

    // Step 4: Remove target from the nodes container.
    nodes.erase(nodes.begin() + targetIndex);

    // Step 5: Update entryPoint.
    if (target == entryPoint)
    {
        if (nodes.empty())
        {
            // Graph is now empty.
            entryPoint = nullptr;
        }
        else
        {
            // Select the remaining node with the highest level.
            // On ties, the first encountered in nodes order wins.
            HNSWNode* best = nodes[0];
            for (int i = 1; i < static_cast<int>(nodes.size()); i++)
            {
                if (nodes[i]->level > best->level)
                {
                    best = nodes[i];
                }
            }
            entryPoint = best;
        }
    }

    // Step 6: Free the deleted node's memory exactly once.
    // All pointers to target have already been removed above.
    delete target;
}

int HNSW::nodeCount() const
{
    return static_cast<int>(nodes.size());
}
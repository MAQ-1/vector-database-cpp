#include "KDNode.h"
#include "KDTree.h"
#include <iostream>
#include <cfloat> 
#include <stdexcept>
#include "Similarity.h"
#include <cmath>
using namespace std;

KDNode::KDNode(const VectorRecord &record)
    : record(record), left(nullptr), right(nullptr) {}

KDTree::KDTree()
{
    root = nullptr;
}

// Public insert function
void KDTree::insert(const VectorRecord &record)
{
    root = insert(root, record, 0);
}

// private insert function
KDNode *KDTree::insert(
    KDNode *node,
    const VectorRecord &record,
    int depth)
{
    // Base Case
    if (node == nullptr)
    {
        return new KDNode(record);
    }

    int axis = depth % record.embedding.size();
    //   new node hai            // ye current node hai 
    if (record.embedding[axis] < node->record.embedding[axis])
    {
        // Go to the left subtree.
        node->left = insert(
            node->left,
            record,
            depth + 1);
    }
    else
    {
        node->right = insert(
            node->right,
            record,
            depth + 1);
    }

    return node;
}

void KDTree::display() const
{
    display(root);
}

void KDTree::display(KDNode *node) const
{
    // base case

    if (node == nullptr)
    {
        return;
    }

    // Visit the current node
    std::cout << "ID: " << node->record.id << std::endl;
    std::cout << "Metadata: " << node->record.metadata << std::endl;

    std::cout << "Embedding: ";

    for (float value : node->record.embedding)
    {
        std::cout << value << " ";
    }
    // Traverse Left Subtree
    display(node->left);

    // Traverse Right Subtree
    display(node->right);
}

VectorRecord KDTree::nearestNeighbor(
    const std::vector<float>&query
){
    // tree is empty 
    if(root == nullptr){
        throw std::runtime_error("KD-Tree is empty.");
    }

    // Initialize bestNode and bestDistance

    KDNode *bestNode = nullptr;
    float bestDistance = FLT_MAX;

    nearestNeighbor(root, query, 0, bestNode, bestDistance);

    return bestNode->record;
}

// private recursive function to find nearest neighbor
// Private recursive function to find the nearest neighbor.
void KDTree::nearestNeighbor(
    KDNode* node,
    const std::vector<float>& query,
    int depth,
    KDNode*& bestNode,
    float& bestDistance)
{
    // Base Case:
    // If the current node is null, stop recursion.
    if (node == nullptr)
    {
        return;
    }

    // Compute Euclidean distance between the query
    // and the current node.
    float distance = Similarity::euclideanDistance(
        query,
        node->record.embedding
    );

    // If the current node is closer,
    // update the best node and best distance.
    if (distance < bestDistance)
    {
        bestDistance = distance;
        bestNode = node;
    }

    // Determine which dimension (axis) to compare.
    // Example:
    // depth 0 -> X
    // depth 1 -> Y
    // depth 2 -> X
    int axis = depth % query.size();

    // Determine which subtree is nearer
    // and which is farther from the query.
    KDNode* nearChild;
    KDNode* farChild;

    if (query[axis] < node->record.embedding[axis])
    {
        nearChild = node->left;
        farChild = node->right;
    }
    else
    {
        nearChild = node->right;
        farChild = node->left;
    }

    // Always search the nearer subtree first.
    nearestNeighbor(
        nearChild,
        query,
        depth + 1,
        bestNode,
        bestDistance
    );

    // Distance from the query to the splitting plane.
    float planeDistance =
        std::abs(query[axis] - node->record.embedding[axis]);

    // Search the farther subtree only if it can
    // potentially contain a closer point.
    if (planeDistance < bestDistance)
    {
        nearestNeighbor(
            farChild,
            query,
            depth + 1,
            bestNode,
            bestDistance
        );
    }
}
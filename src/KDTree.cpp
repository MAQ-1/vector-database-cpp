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
    const std::vector<float> &query)
{
    // tree is empty
    if (root == nullptr)
    {
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
    KDNode *node,
    const std::vector<float> &query,
    int depth,
    KDNode *&bestNode,
    float &bestDistance)
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
        node->record.embedding);

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
    KDNode *nearChild;
    KDNode *farChild;

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
        bestDistance);

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
            bestDistance);
    }
}

// remov e

KDNode *KDTree::findMin(
    KDNode *node,
    int targetDimension,
    int depth)
{
    if (node == nullptr)
    {
        return nullptr;
    }

    int currentDimension = depth % node->record.embedding.size();

    if (currentDimension == targetDimension)
    {
        if (node->left == nullptr)
        {
            return node;
        }

        return findMin(node->left, targetDimension, depth + 1);
    }

    KDNode *leftMin =
        findMin(
            node->left,
            targetDimension,
            depth + 1);

    KDNode *rightMin =
        findMin(
            node->right,
            targetDimension,
            depth + 1);

    // Return the node with the minimum value in the specified dimension.
    KDNode *minimum = node;
    if (leftMin != nullptr &&
        leftMin->record.embedding[targetDimension] <
            minimum->record.embedding[targetDimension])
    {
        minimum = leftMin;
    }
    if (rightMin != nullptr &&
        rightMin->record.embedding[targetDimension] <
            minimum->record.embedding[targetDimension])
    {
        minimum = rightMin;
    }
    return minimum;
}

KDNode *KDTree::findMax(
    KDNode *node,
    int targetDimension,
    int depth)
{
    // Base Case
    if (node == nullptr)
    {
        return nullptr;
    }

    int currentDimension =
        depth % node->record.embedding.size();

    // If the current splitting dimension matches
    // the target dimension, maximum can only be
    // in the current node or right subtree.
    if (currentDimension == targetDimension)
    {
        if (node->right == nullptr)
        {
            return node;
        }

        return findMax(
            node->right,
            targetDimension,
            depth + 1);
    }

    // Otherwise search both subtrees.
    KDNode *leftMax =
        findMax(
            node->left,
            targetDimension,
            depth + 1);

    KDNode *rightMax =
        findMax(
            node->right,
            targetDimension,
            depth + 1);

    // Find the maximum among current, left and right.
    KDNode *maximum = node;

    if (leftMax != nullptr &&
        leftMax->record.embedding[targetDimension] >
            maximum->record.embedding[targetDimension])
    {
        maximum = leftMax;
    }

    if (rightMax != nullptr &&
        rightMax->record.embedding[targetDimension] >
            maximum->record.embedding[targetDimension])
    {
        maximum = rightMax;
    }

    return maximum;
}

// remove node by id
KDNode* KDTree::removeNode(
    KDNode* node,
    int id,
    int depth)
{
    // Base Case
    if (node == nullptr)
    {
        return nullptr;
    }

    int currentDimension =
        depth % node->record.embedding.size();

    // Found the node to delete
    if (node->record.id == id)
    {
        // Case 1: Leaf Node
        if (node->left == nullptr &&
            node->right == nullptr)
        {
            delete node;
            return nullptr;
        }

        // Case 2: Right subtree exists
        if (node->right != nullptr)
        {
            KDNode* replacement =
                findMin(
                    node->right,
                    currentDimension,
                    depth + 1);

            node->record = replacement->record;

            node->right =
                removeNode(
                    node->right,
                    replacement->record.id,
                    depth + 1);
        }

        // Case 3: Only left subtree exists
        else
        {
            KDNode* replacement =
                findMax(
                    node->left,
                    currentDimension,
                    depth + 1);

            node->record = replacement->record;

            node->left =
                removeNode(
                    node->left,
                    replacement->record.id,
                    depth + 1);
        }
    }
    else
    {
        // Since we only know the ID, search both subtrees.
        node->left =
            removeNode(
                node->left,
                id,
                depth + 1);

        node->right =
            removeNode(
                node->right,
                id,
                depth + 1);
    }

    return node;
}
#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include <stdexcept>
#include <unordered_set>
#include <unordered_map>

#include "HNSW.h"
#include "HNSWNode.h"
#include "VectorRecord.h"

// -------------------------------------------------------
// makeRecord
// 4-dim vector, all components equal to val.
// -------------------------------------------------------
static VectorRecord makeRecord(int id, float val)
{
    return VectorRecord(id, {val, val, val, val}, "test");
}

// -------------------------------------------------------
// noReferenceToDeletedPtr
//
// Walks every reachable node from entryPoint at ALL levels
// and checks that none of their neighbor lists contain the
// address `deletedPtr`.
//
// IMPORTANT: does NOT dereference deletedPtr.
// The pointer was freed by remove(). We only compare
// addresses. Dereferencing a freed pointer is UB and would
// make the test itself the source of the bug it is testing.
//
// Returns true  → no reachable neighbor list holds deletedPtr.
// Returns false → at least one neighbor list still holds it
//                 (dangling pointer bug detected).
// -------------------------------------------------------
static bool noReferenceToDeletedPtr(HNSW& hnsw, const HNSWNode* deletedPtr)
{
    HNSWNode* ep = hnsw.getEntryPoint();
    if (ep == nullptr) return true;

    std::unordered_set<HNSWNode*> visited;
    std::vector<HNSWNode*> queue = {ep};

    while (!queue.empty())
    {
        HNSWNode* cur = queue.back();
        queue.pop_back();

        if (!visited.insert(cur).second) continue;

        for (int lvl = 0; lvl <= cur->level; lvl++)
        {
            for (HNSWNode* nb : cur->neighbors[lvl])
            {
                // Address comparison only — never dereference deletedPtr.
                if (nb == deletedPtr) return false;

                if (visited.find(nb) == visited.end())
                    queue.push_back(nb);
            }
        }
    }
    return true;
}

// -------------------------------------------------------
// maxLevelReachable
//
// Returns the maximum `level` value among all nodes
// reachable from entryPoint. Used to verify entryPoint
// selection rule after deletion.
// -------------------------------------------------------
static int maxLevelReachable(HNSW& hnsw)
{
    HNSWNode* ep = hnsw.getEntryPoint();
    if (ep == nullptr) return -1;

    int maxLvl = ep->level;
    std::unordered_set<HNSWNode*> visited;
    std::vector<HNSWNode*> queue = {ep};

    while (!queue.empty())
    {
        HNSWNode* cur = queue.back();
        queue.pop_back();

        if (!visited.insert(cur).second) continue;

        if (cur->level > maxLvl) maxLvl = cur->level;

        for (HNSWNode* nb : cur->neighbors[0])
        {
            if (visited.find(nb) == visited.end())
                queue.push_back(nb);
        }
    }
    return maxLvl;
}

// -------------------------------------------------------
// TEST 1: Delete an arbitrary node — verify core invariants
//
// Does NOT assume the deleted node is a leaf or internal.
// HNSW topology is random; the invariants hold regardless.
// -------------------------------------------------------
static void test1_delete_arbitrary_node()
{
    HNSW hnsw;
    for (int i = 1; i <= 5; i++)
        hnsw.insert(makeRecord(i, static_cast<float>(i * 10)));

    assert(hnsw.nodeCount() == 5);

    // Capture the raw address of the node BEFORE deletion.
    // We need the address to check for dangling pointers.
    // We do NOT use this pointer after remove() — only compare addresses.
    HNSWNode* epBefore = hnsw.getEntryPoint();
    int epIdBefore = epBefore->record.id;

    // Delete id=1. If id=1 happens to be the entryPoint, the
    // entryPoint-deletion path is exercised (covered fully in test3).
    // Either way, the invariants below must hold.
    HNSWNode* nodeToDelete = nullptr;
    {
        // Walk reachable nodes to find the pointer for id=1.
        // We need the address before deletion to check for dangling refs.
        std::unordered_set<HNSWNode*> visited;
        std::vector<HNSWNode*> queue = {epBefore};
        while (!queue.empty() && nodeToDelete == nullptr)
        {
            HNSWNode* cur = queue.back(); queue.pop_back();
            if (!visited.insert(cur).second) continue;
            if (cur->record.id == 1) { nodeToDelete = cur; break; }
            for (HNSWNode* nb : cur->neighbors[0])
                if (visited.find(nb) == visited.end()) queue.push_back(nb);
        }
    }

    hnsw.remove(1);

    // Node count must decrease by exactly 1.
    assert(hnsw.nodeCount() == 4);

    // EntryPoint must still be valid (non-null, since 4 nodes remain).
    assert(hnsw.getEntryPoint() != nullptr);

    // If we found the pointer before deletion, verify no dangling reference.
    if (nodeToDelete != nullptr)
        assert(noReferenceToDeletedPtr(hnsw, nodeToDelete));

    // Search must not crash and must not return id=1.
    VectorRecord r = hnsw.search({10.f, 10.f, 10.f, 10.f});
    assert(r.id != 1);

    std::cout << "TEST 1 PASSED: delete arbitrary node, invariants hold\n";
}

// -------------------------------------------------------
// TEST 2: Delete every node one by one — invariants hold
//         at each step regardless of topology
//
// Replaces the "likely internal" assumption with a loop
// that exercises deletion at every possible topology
// position (leaf, internal, entryPoint) without assuming
// which is which.
// -------------------------------------------------------
static void test2_delete_all_nodes_sequentially()
{
    HNSW hnsw;
    const int N = 10;
    for (int i = 1; i <= N; i++)
        hnsw.insert(makeRecord(i, static_cast<float>(i * 10)));

    assert(hnsw.nodeCount() == N);

    for (int i = 1; i <= N; i++)
    {
        int countBefore = hnsw.nodeCount();

        // Capture the pointer for id=i before deletion.
        HNSWNode* ep = hnsw.getEntryPoint();
        HNSWNode* targetPtr = nullptr;
        {
            std::unordered_set<HNSWNode*> visited;
            std::vector<HNSWNode*> queue = {ep};
            while (!queue.empty() && targetPtr == nullptr)
            {
                HNSWNode* cur = queue.back(); queue.pop_back();
                if (!visited.insert(cur).second) continue;
                if (cur->record.id == i) { targetPtr = cur; break; }
                for (HNSWNode* nb : cur->neighbors[0])
                    if (visited.find(nb) == visited.end()) queue.push_back(nb);
            }
        }

        hnsw.remove(i);

        // Count decreased by exactly 1.
        assert(hnsw.nodeCount() == countBefore - 1);

        if (hnsw.nodeCount() == 0)
        {
            // Last node deleted — entryPoint must be nullptr.
            assert(hnsw.getEntryPoint() == nullptr);
        }
        else
        {
            // Graph still has nodes — entryPoint must be valid.
            assert(hnsw.getEntryPoint() != nullptr);

            // No dangling pointer to the deleted node.
            if (targetPtr != nullptr)
                assert(noReferenceToDeletedPtr(hnsw, targetPtr));

            // Search must not crash.
            hnsw.search({50.f, 50.f, 50.f, 50.f});
        }
    }

    std::cout << "TEST 2 PASSED: delete all nodes one by one, invariants hold at each step\n";
}

// -------------------------------------------------------
// TEST 3: Delete the current entryPoint explicitly
//
// Deterministically identifies the entryPoint, deletes it,
// and verifies the replacement rule:
//   new entryPoint level >= level of every other remaining node.
// -------------------------------------------------------
static void test3_delete_entry_point()
{
    HNSW hnsw;
    for (int i = 1; i <= 10; i++)
        hnsw.insert(makeRecord(i, static_cast<float>(i * 10)));

    HNSWNode* oldEP = hnsw.getEntryPoint();
    assert(oldEP != nullptr);

    // Capture id and address before deletion.
    int epId = oldEP->record.id;
    const HNSWNode* oldEPAddr = oldEP;

    int countBefore = hnsw.nodeCount();

    hnsw.remove(epId);

    // Count decreased by 1.
    assert(hnsw.nodeCount() == countBefore - 1);

    // New entryPoint must exist and must not be the deleted node.
    HNSWNode* newEP = hnsw.getEntryPoint();
    assert(newEP != nullptr);
    assert(newEP != oldEPAddr); // address comparison — not a dereference of freed memory

    // New entryPoint must satisfy the selection rule:
    // its level must be >= the level of every other remaining reachable node.
    int maxLvl = maxLevelReachable(hnsw);
    assert(newEP->level == maxLvl);

    // No reachable neighbor list holds the old entryPoint's address.
    assert(noReferenceToDeletedPtr(hnsw, oldEPAddr));

    // Search must still work.
    hnsw.search({50.f, 50.f, 50.f, 50.f});

    std::cout << "TEST 3 PASSED: delete entryPoint, new entryPoint satisfies selection rule\n";
}

// -------------------------------------------------------
// TEST 4: Delete the only node
// -------------------------------------------------------
static void test4_delete_only_node()
{
    HNSW hnsw;
    hnsw.insert(makeRecord(42, 1.0f));
    assert(hnsw.nodeCount() == 1);

    hnsw.remove(42);

    assert(hnsw.nodeCount() == 0);
    assert(hnsw.getEntryPoint() == nullptr);

    // search() on empty graph must throw std::runtime_error.
    // This is the existing convention in HNSW.cpp:
    //   if (nodes.empty()) throw std::runtime_error("HNSW graph is empty.");
    bool threw = false;
    try { hnsw.search({1.f, 1.f, 1.f, 1.f}); }
    catch (const std::runtime_error&) { threw = true; }
    assert(threw);

    std::cout << "TEST 4 PASSED: delete only node → nodeCount=0, entryPoint=nullptr, search throws\n";
}

// -------------------------------------------------------
// TEST 5: Delete a nonexistent ID — graph must be unchanged
// -------------------------------------------------------
static void test5_delete_nonexistent()
{
    HNSW hnsw;
    for (int i = 1; i <= 5; i++)
        hnsw.insert(makeRecord(i, static_cast<float>(i * 10)));

    int countBefore = hnsw.nodeCount();
    HNSWNode* epBefore = hnsw.getEntryPoint();

    // 999 does not exist.
    hnsw.remove(999);

    // Nothing must have changed.
    assert(hnsw.nodeCount() == countBefore);
    assert(hnsw.getEntryPoint() == epBefore); // same pointer, same node

    // Graph still searchable.
    hnsw.search({25.f, 25.f, 25.f, 25.f});

    std::cout << "TEST 5 PASSED: delete nonexistent id is a strict no-op\n";
}

// -------------------------------------------------------
// TEST 6: Sequential deletions — insert 1..5, remove 5,4,2,1
//
// After all four deletions only node 3 remains.
// node 3 is the ONLY remaining node — this is a logical
// certainty, not a topology assumption.
// -------------------------------------------------------
static void test6_sequential_deletions()
{
    HNSW hnsw;
    for (int i = 1; i <= 5; i++)
        hnsw.insert(makeRecord(i, static_cast<float>(i * 10)));

    // Capture addresses before any deletion.
    // Walk from entryPoint to collect all node pointers.
    std::unordered_map<int, HNSWNode*> addrById;
    {
        std::unordered_set<HNSWNode*> visited;
        std::vector<HNSWNode*> queue = {hnsw.getEntryPoint()};
        while (!queue.empty())
        {
            HNSWNode* cur = queue.back(); queue.pop_back();
            if (!visited.insert(cur).second) continue;
            addrById[cur->record.id] = cur;
            for (HNSWNode* nb : cur->neighbors[0])
                if (visited.find(nb) == visited.end()) queue.push_back(nb);
        }
    }

    hnsw.remove(5); assert(hnsw.nodeCount() == 4);
    hnsw.remove(4); assert(hnsw.nodeCount() == 3);
    hnsw.remove(2); assert(hnsw.nodeCount() == 2);
    hnsw.remove(1); assert(hnsw.nodeCount() == 1);

    // Only node 3 remains — this is certain, not probabilistic.
    HNSWNode* ep = hnsw.getEntryPoint();
    assert(ep != nullptr);
    assert(ep->record.id == 3);

    // Verify no dangling pointers to any of the four deleted nodes.
    for (int deletedId : {1, 2, 4, 5})
    {
        auto it = addrById.find(deletedId);
        if (it != addrById.end())
            assert(noReferenceToDeletedPtr(hnsw, it->second));
    }

    // Search must return node 3 — it is the only node.
    VectorRecord r = hnsw.search({30.f, 30.f, 30.f, 30.f});
    assert(r.id == 3);

    std::cout << "TEST 6 PASSED: sequential deletions, only node 3 remains, no dangling pointers\n";
}

// -------------------------------------------------------
// TEST 7: Delete then insert — no stale pointer, new node
//         is reachable and returned by search
// -------------------------------------------------------
static void test7_delete_then_insert()
{
    HNSW hnsw;
    hnsw.insert(makeRecord(1, 10.f));
    hnsw.insert(makeRecord(2, 20.f));
    hnsw.insert(makeRecord(3, 30.f));

    // Capture address of node 2 before deletion.
    HNSWNode* node2Addr = nullptr;
    {
        std::unordered_set<HNSWNode*> visited;
        std::vector<HNSWNode*> queue = {hnsw.getEntryPoint()};
        while (!queue.empty() && node2Addr == nullptr)
        {
            HNSWNode* cur = queue.back(); queue.pop_back();
            if (!visited.insert(cur).second) continue;
            if (cur->record.id == 2) { node2Addr = cur; break; }
            for (HNSWNode* nb : cur->neighbors[0])
                if (visited.find(nb) == visited.end()) queue.push_back(nb);
        }
    }

    hnsw.remove(2);
    assert(hnsw.nodeCount() == 2);

    // No stale pointer to node 2 after deletion.
    if (node2Addr != nullptr)
        assert(noReferenceToDeletedPtr(hnsw, node2Addr));

    // Insert node 4 — must succeed and be connected to the graph.
    hnsw.insert(makeRecord(4, 40.f));
    assert(hnsw.nodeCount() == 3);
    assert(hnsw.getEntryPoint() != nullptr);

    // Still no stale pointer to node 2 after the new insert.
    if (node2Addr != nullptr)
        assert(noReferenceToDeletedPtr(hnsw, node2Addr));

    // Node 4 is the closest to {40,40,40,40} among {1,3,4}.
    // This verifies node 4 is actually reachable and returned by search.
    VectorRecord r = hnsw.search({40.f, 40.f, 40.f, 40.f});
    assert(r.id == 4);

    std::cout << "TEST 7 PASSED: delete then insert, node 4 reachable, no stale pointer to node 2\n";
}

// -------------------------------------------------------
// main
// -------------------------------------------------------
int main()
{
    test1_delete_arbitrary_node();
    test2_delete_all_nodes_sequentially();
    test3_delete_entry_point();
    test4_delete_only_node();
    test5_delete_nonexistent();
    test6_sequential_deletions();
    test7_delete_then_insert();

    std::cout << "\nAll HNSW remove() tests passed.\n";
    return 0;
}

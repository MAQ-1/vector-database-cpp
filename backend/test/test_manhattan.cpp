#include <iostream>
#include <vector>
#include <cassert>
#include <cmath>
#include <stdexcept>

#include "VectorDatabase.h"
#include "VectorRecord.h"
#include "Metric.h"

using namespace std;

// -------------------------------------------------------
// Helpers
// -------------------------------------------------------

static float manhattanExpected(const vector<float>& a, const vector<float>& b)
{
    float sum = 0.0f;
    for (size_t i = 0; i < a.size(); i++)
        sum += abs(a[i] - b[i]);
    return sum;
}

static const float EPSILON = 1e-4f;

static bool nearlyEqual(float a, float b)
{
    return abs(a - b) < EPSILON;
}

// -------------------------------------------------------
// TEST 1: Manhattan score is computed correctly
//
// Insert 3 records with known embeddings.
// Query with a known vector.
// Verify each returned score matches the hand-computed
// Manhattan distance exactly.
// -------------------------------------------------------
static void test1_manhattan_scores_are_correct()
{
    VectorDatabase db;
    db.insert(VectorRecord(1, {1.0f, 2.0f, 3.0f}, "a"));
    db.insert(VectorRecord(2, {4.0f, 5.0f, 6.0f}, "a"));
    db.insert(VectorRecord(3, {7.0f, 8.0f, 9.0f}, "a"));

    vector<float> query = {1.0f, 1.0f, 1.0f};

    auto results = db.knnSearch(query, 3, Metric::MANHATTAN);
    assert(results.size() == 3);

    // Verify each score against hand-computed Manhattan distance.
    for (const auto& r : results)
    {
        vector<float> emb;
        if (r.record.id == 1) emb = {1.0f, 2.0f, 3.0f};
        else if (r.record.id == 2) emb = {4.0f, 5.0f, 6.0f};
        else                       emb = {7.0f, 8.0f, 9.0f};

        float expected = manhattanExpected(query, emb);
        assert(nearlyEqual(r.score, expected));
    }

    cout << "TEST 1 PASSED: Manhattan scores match hand-computed values\n";
}

// -------------------------------------------------------
// TEST 2: Nearest result is the record with smallest
//         Manhattan distance to the query
// -------------------------------------------------------
static void test2_nearest_result_ordering()
{
    VectorDatabase db;
    // id=1 is closest to query {0,0,0}: distance = 3
    // id=2: distance = 12
    // id=3: distance = 30
    db.insert(VectorRecord(1, {1.0f, 1.0f, 1.0f}, "a"));
    db.insert(VectorRecord(2, {4.0f, 4.0f, 4.0f}, "a"));
    db.insert(VectorRecord(3, {10.0f, 10.0f, 10.0f}, "a"));

    vector<float> query = {0.0f, 0.0f, 0.0f};

    auto results = db.knnSearch(query, 1, Metric::MANHATTAN);
    assert(results.size() == 1);
    assert(results[0].record.id == 1);
    assert(nearlyEqual(results[0].score, 3.0f));

    cout << "TEST 2 PASSED: nearest result has smallest Manhattan distance\n";
}

// -------------------------------------------------------
// TEST 3: Top-K results are ordered ascending by distance
//         (smallest distance first)
// -------------------------------------------------------
static void test3_topk_ascending_order()
{
    VectorDatabase db;
    db.insert(VectorRecord(1, {1.0f, 0.0f}, "a"));   // dist to {0,0} = 1
    db.insert(VectorRecord(2, {3.0f, 0.0f}, "a"));   // dist = 3
    db.insert(VectorRecord(3, {5.0f, 0.0f}, "a"));   // dist = 5
    db.insert(VectorRecord(4, {2.0f, 0.0f}, "a"));   // dist = 2
    db.insert(VectorRecord(5, {4.0f, 0.0f}, "a"));   // dist = 4

    vector<float> query = {0.0f, 0.0f};

    auto results = db.knnSearch(query, 5, Metric::MANHATTAN);
    assert(results.size() == 5);

    // Scores must be non-decreasing.
    for (size_t i = 1; i < results.size(); i++)
        assert(results[i].score >= results[i - 1].score);

    // Exact order: id 1,4,2,5,3 (distances 1,2,3,4,5).
    assert(results[0].record.id == 1);
    assert(results[1].record.id == 4);
    assert(results[2].record.id == 2);
    assert(results[3].record.id == 5);
    assert(results[4].record.id == 3);

    cout << "TEST 3 PASSED: Top-K results are in ascending distance order\n";
}

// -------------------------------------------------------
// TEST 4: knnSearchOptimized returns the same IDs and
//         scores as knnSearch for Manhattan metric.
//
// All records have STRICTLY DISTINCT distances to the query
// so positional ordering is fully determined in both
// implementations. No ties exist that could cause a valid
// but differently-ordered result to fail the assertion.
//
// Hand-verified distances (query = {1,1,1}):
//   id=1  {1,2,3}  → |0|+|1|+|2| = 3
//   id=2  {4,1,0}  → |3|+|0|+|1| = 4
//   id=3  {0,5,0}  → |1|+|4|+|1| = 6
//   id=4  {9,9,9}  → |8|+|8|+|8| = 24
//   id=5  {3,1,1}  → |2|+|0|+|0| = 2
// Top-3 expected order: id5(2), id1(3), id2(4)
// -------------------------------------------------------
static void test4_optimized_matches_sort_based()
{
    VectorDatabase db;
    db.insert(VectorRecord(1, {1.0f, 2.0f, 3.0f}, "a"));
    db.insert(VectorRecord(2, {4.0f, 1.0f, 0.0f}, "a"));
    db.insert(VectorRecord(3, {0.0f, 5.0f, 0.0f}, "a"));
    db.insert(VectorRecord(4, {9.0f, 9.0f, 9.0f}, "a"));
    db.insert(VectorRecord(5, {3.0f, 1.0f, 1.0f}, "a"));

    vector<float> query = {1.0f, 1.0f, 1.0f};
    const int k = 3;

    auto sortBased = db.knnSearch(query, k, Metric::MANHATTAN);
    auto heapBased = db.knnSearchOptimized(query, k, Metric::MANHATTAN);

    assert(sortBased.size() == static_cast<size_t>(k));
    assert(heapBased.size() == static_cast<size_t>(k));

    // Print both result sets for visibility.
    cout << "  NORMAL KNN:    ";
    for (int i = 0; i < k; i++)
        cout << "id=" << sortBased[i].record.id << " score=" << sortBased[i].score << "  ";
    cout << "\n";

    cout << "  OPTIMIZED KNN: ";
    for (int i = 0; i < k; i++)
        cout << "id=" << heapBased[i].record.id << " score=" << heapBased[i].score << "  ";
    cout << "\n";

    // All distances are distinct so positional order must agree exactly.
    for (int i = 0; i < k; i++)
    {
        assert(sortBased[i].record.id == heapBased[i].record.id);
        assert(nearlyEqual(sortBased[i].score, heapBased[i].score));
    }

    // Verify the expected order explicitly.
    assert(sortBased[0].record.id == 5); assert(nearlyEqual(sortBased[0].score, 2.0f));
    assert(sortBased[1].record.id == 1); assert(nearlyEqual(sortBased[1].score, 3.0f));
    assert(sortBased[2].record.id == 2); assert(nearlyEqual(sortBased[2].score, 4.0f));

    cout << "TEST 4 PASSED: knnSearchOptimized matches knnSearch for Manhattan\n";
}

// -------------------------------------------------------
// TEST 5: Dimension mismatch still throws
//
// Similarity::manhattanDistance throws invalid_argument
// when vectors have different sizes. Verify this propagates.
// -------------------------------------------------------
static void test5_dimension_mismatch_throws()
{
    VectorDatabase db;
    db.insert(VectorRecord(1, {1.0f, 2.0f, 3.0f}, "a"));

    // Query has 2 dimensions, record has 3.
    vector<float> query = {1.0f, 2.0f};

    bool threw = false;
    try { db.knnSearch(query, 1, Metric::MANHATTAN); }
    catch (const invalid_argument&) { threw = true; }
    assert(threw);

    threw = false;
    try { db.knnSearchOptimized(query, 1, Metric::MANHATTAN); }
    catch (const invalid_argument&) { threw = true; }
    assert(threw);

    cout << "TEST 5 PASSED: dimension mismatch throws invalid_argument\n";
}

// -------------------------------------------------------
// TEST 6: Existing EUCLIDEAN behavior is unchanged
//
// Verifies that adding MANHATTAN did not alter the sort
// direction or score values for EUCLIDEAN.
// -------------------------------------------------------
static void test6_euclidean_behavior_unchanged()
{
    VectorDatabase db;
    db.insert(VectorRecord(1, {1.0f, 0.0f}, "a"));   // euclidean dist to {0,0} = 1
    db.insert(VectorRecord(2, {3.0f, 0.0f}, "a"));   // dist = 3
    db.insert(VectorRecord(3, {2.0f, 0.0f}, "a"));   // dist = 2

    vector<float> query = {0.0f, 0.0f};

    auto results = db.knnSearch(query, 3, Metric::EUCLIDEAN);
    assert(results.size() == 3);

    // Must still be ascending.
    assert(results[0].record.id == 1);
    assert(results[1].record.id == 3);
    assert(results[2].record.id == 2);

    for (size_t i = 1; i < results.size(); i++)
        assert(results[i].score >= results[i - 1].score);

    cout << "TEST 6 PASSED: EUCLIDEAN behavior unchanged after adding MANHATTAN\n";
}

// -------------------------------------------------------
// TEST 7: Existing COSINE behavior is unchanged
//
// Verifies that adding MANHATTAN did not alter the sort
// direction for COSINE (larger score must still be first).
// -------------------------------------------------------
static void test7_cosine_behavior_unchanged()
{
    VectorDatabase db;
    // id=1 is most aligned with query {1,0}: cosine ~ 1.0
    // id=2 is orthogonal: cosine ~ 0.0
    db.insert(VectorRecord(1, {1.0f, 0.0f}, "a"));
    db.insert(VectorRecord(2, {0.0f, 1.0f}, "a"));

    vector<float> query = {1.0f, 0.0f};

    auto results = db.knnSearch(query, 2, Metric::COSINE);
    assert(results.size() == 2);

    // Largest cosine similarity must be first.
    assert(results[0].score >= results[1].score);
    assert(results[0].record.id == 1);

    cout << "TEST 7 PASSED: COSINE behavior unchanged after adding MANHATTAN\n";
}

// -------------------------------------------------------
// main
// -------------------------------------------------------
int main()
{
    test1_manhattan_scores_are_correct();
    test2_nearest_result_ordering();
    test3_topk_ascending_order();
    test4_optimized_matches_sort_based();
    test5_dimension_mismatch_throws();
    test6_euclidean_behavior_unchanged();
    test7_cosine_behavior_unchanged();

    cout << "\nAll Manhattan metric tests passed.\n";
    return 0;
}

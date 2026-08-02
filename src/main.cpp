#include <iostream>
#include <vector>
#include <algorithm>
#include "VectorDatabase.h"
#include "VectorRecord.h"
#include "Metric.h"
#include <chrono>
#include <random>
#include "KDTree.h"

using namespace std;

int main()
{
    VectorDatabase db;
    KDTree tree;

    // Number of vectors to generate
    const int NUM_VECTORS = 100000;

    // Random number generator
    std::mt19937 rng(std::random_device{}());

    // Generate values between 0 and 1000
    std::uniform_real_distribution<float> dist(0.0f, 1000.0f);

    // ===========================
    // Generate Random Dataset
    // ===========================

    for (int i = 1; i <= NUM_VECTORS; i++)
    {
        std::vector<float> embedding =
        {
            dist(rng),
            dist(rng)
        };

        VectorRecord record(
            i,
            embedding,
            "Random"
        );

        db.insert(record);
        tree.insert(record);
    }

    std::cout << "Dataset Size : "
              << NUM_VECTORS
              << " vectors\n\n";

    // Random query vector
    std::vector<float> query =
    {
        dist(rng),
        dist(rng)
    };

    // =========================================
    // Brute Force Benchmark
    // =========================================

    auto start1 = std::chrono::high_resolution_clock::now();

    VectorRecord bruteResult =
        db.search(query);

    auto end1 = std::chrono::high_resolution_clock::now();

    auto bruteTime =
    std::chrono::duration_cast<
        std::chrono::microseconds>(end1 - start1);

    // =========================================
    // KD Tree Benchmark
    // =========================================

    auto start2 = std::chrono::high_resolution_clock::now();

    VectorRecord kdResult =
        tree.nearestNeighbor(query);

    auto end2 = std::chrono::high_resolution_clock::now();

    auto kdTime =
    std::chrono::duration_cast<
        std::chrono::microseconds>(end2 - start2);
    // =========================================
    // Results
    // =========================================

    std::cout << "========== Brute Force ==========\n";
    std::cout << "Nearest ID : "
              << bruteResult.id
              << std::endl;

    std::cout << "Time : "
              << bruteTime.count()
              << " ms\n\n";

    std::cout << "========== KD Tree ==========\n";
    std::cout << "Nearest ID : "
              << kdResult.id
              << std::endl;

    std::cout << "Time : "
              << kdTime.count()
              << " ms\n";

    return 0;
}
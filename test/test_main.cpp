#include <iostream>
#include "VectorDatabase.h"

int main()
{
    VectorDatabase db;

for (int i = 0; i < 10000; i++)
{
    std::vector<float> embedding = {
        static_cast<float>(rand() % 1000),
        static_cast<float>(rand() % 1000)
    };

    db.insert(VectorRecord(i, embedding, "Random"));
}

    std::vector<float> query = {8.1f,8.2f};

    // Benchmark
    Benchmark result = db.benchmark(query);

    std::cout << "\n========== Benchmark ==========\n";
    std::cout << "Brute Force : " << result.bruteForceTime << " us\n";
    std::cout << "KD-Tree     : " << result.kdTreeTime << " us\n";
    std::cout << "LSH         : " << result.lshTime << " us\n";
    std::cout << "HNSW        : " << result.hnswTime << " us\n";

    // Verify correctness
    std::cout << "\n========== Search Results ==========\n";

    std::cout << "Brute Force : "
              << db.search(query, SearchAlgorithm::BRUTE_FORCE).id
              << '\n';

    std::cout << "KD-Tree     : "
              << db.search(query, SearchAlgorithm::KD_TREE).id
              << '\n';

    std::cout << "LSH         : "
              << db.search(query, SearchAlgorithm::LSH).id
              << '\n';

    std::cout << "HNSW        : "
              << db.search(query, SearchAlgorithm::HNSW).id
              << '\n';

    return 0;
}
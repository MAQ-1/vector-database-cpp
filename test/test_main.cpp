#include <iostream>
#include "VectorDatabase.h"

using namespace std;

int main()
{
    VectorDatabase db;

for (int i = 0; i < 10000; i++)
{
    vector<float> embedding = {
        static_cast<float>(rand() % 1000),
        static_cast<float>(rand() % 1000)
    };

    db.insert(VectorRecord(i, embedding, "Random"));
}

    vector<float> query = {8.1f,8.2f};

    // Benchmark
    Benchmark result = db.benchmark(query);

    cout << "\n========== Benchmark ==========\n";
    cout << "Brute Force : " << result.bruteForceTime << " us\n";
    cout << "KD-Tree     : " << result.kdTreeTime << " us\n";
    cout << "LSH         : " << result.lshTime << " us\n";
    cout << "HNSW        : " << result.hnswTime << " us\n";

    // Verify correctness
    cout << "\n========== Search Results ==========\n";

    cout << "Brute Force : "
              << db.search(query, SearchAlgorithm::BRUTE_FORCE).id
              << '\n';

    cout << "KD-Tree     : "
              << db.search(query, SearchAlgorithm::KD_TREE).id
              << '\n';

    cout << "LSH         : "
              << db.search(query, SearchAlgorithm::LSH).id
              << '\n';

    cout << "HNSW        : "
              << db.search(query, SearchAlgorithm::HNSW).id
              << '\n';

    return 0;
}
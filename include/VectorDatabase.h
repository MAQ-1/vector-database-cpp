#ifndef VECTOR_DATABASE_H
#define VECTOR_DATABASE_H
#include "Benchmark.h"
#include <vector>
#include "VectorRecord.h"
#include "SearchResult.h"
#include "Metric.h"
#include "KDTree.h"
#include "HNSW.h"
#include "LSH.h"

enum class SearchAlgorithm
{
    BRUTE_FORCE,
    KD_TREE,
    HNSW,
    LSH
};

class VectorDatabase
{
private:
    std::vector<VectorRecord> records;

    KDTree kdTree;

    HNSW hnsw;

    LSH lsh;

public:
    void insert(const VectorRecord &record);

    void remove(int id);

    VectorRecord search(const std::vector<float> &query);

    VectorRecord search(
        const std::vector<float> &query,
        SearchAlgorithm algorithm);

    // save the database to a file
    // Saving only reads the database.
    void saveToFile(const std::string &filename) const;

    //   load the database from a file
    //    Loading changes the database.
    void loadFromFile(const std::string &filename);

    // It means this function promises not to modify the database.
    void display() const;

    //  KNN

    std::vector<SearchResult> knnSearch(
        const std::vector<float> &query,
        int k,
        Metric metric,
        const std::string &metadataFilter = "");

    std::vector<SearchResult> knnSearchOptimized(
        const std::vector<float> &query,
        int k,
        Metric metric,
        const std::string &metadataFilter = "");

    Benchmark benchmark(
        const std::vector<float> &query);
};

#endif
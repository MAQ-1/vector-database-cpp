#include "VectorDatabase.h"
#include "Similarity.h"
#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <queue>
#include "SearchResult.h"
#include "Metric.h"
#include "Benchmark.h"
#include <chrono>
#include <random>

void VectorDatabase::insert(const VectorRecord &record)
{
    records.emplace_back(record);
    // Update every index.
    kdTree.insert(record);

    hnsw.insert(record);

    lsh.insert(record);
}

// display all records in the database
void VectorDatabase::display() const
{
    std::cout << "\n===== Vector Database =====\n";

    for (const auto &record : records)
    {
        std::cout << "ID: " << record.id << std::endl;

        std::cout << "Embedding: ";

        for (float value : record.embedding)
        {
            std::cout << value << " ";
        }

        std::cout << std::endl;

        std::cout << "Metadata: " << record.metadata << std::endl;
        std::cout << "------------------------" << std::endl;
    }
}

void VectorDatabase::clear()
{
    records.clear();
}
// Remove a record by its ID.

void VectorDatabase::remove(int id)
{
    // Remove from the records vector
    records.erase(
        std::remove_if(
            records.begin(),
            records.end(),
            [id](const VectorRecord &record)
            {
                return record.id == id;
            }),
        records.end());

    // Remove from KD-Tree
    kdTree.remove(id);

    // Remove from LSH
    lsh.remove(id);

    // Remove from HNSW
    hnsw.remove(id);
}

VectorRecord VectorDatabase::search(const std::vector<float> &query,
                                    SearchAlgorithm algorithm)

{
    switch (algorithm)
    {
    case SearchAlgorithm::BRUTE_FORCE:
    {
        auto result = knnSearch(query, 1, Metric::EUCLIDEAN);
        if (result.empty())
        {
            throw std::runtime_error("No records found in the database.");
        }

        return result[0].record;
    }

    case SearchAlgorithm::KD_TREE:
    {
        return kdTree.nearestNeighbor(query);
    }

    case SearchAlgorithm::HNSW:
    {
        return hnsw.search(query);
    }

    case SearchAlgorithm::LSH:
    {
        return lsh.search(query);
    }

    default:
        throw std::invalid_argument("Unsupported search algorithm.");
    }
}

// save the database to a file sirf read krega
void VectorDatabase::saveToFile(const std::string &filename) const
{
    std::ofstream file(filename);

    if (!file.is_open())
    {
        throw std::runtime_error("Unable to open file for writing.");
    }

    for (const auto &record : records)
    {
        file << record.id << "|";

        for (size_t i = 0; i < record.embedding.size(); i++)
        {
            file << record.embedding[i];

            if (i != record.embedding.size() - 1)
            {
                file << ",";
            }
        }

        file << "|" << record.metadata << '\n';
    }
}

// load the file
void VectorDatabase::loadFromFile(const std::string &filename)
{
    std::ifstream file(filename);

    if (!file.is_open())
    {
        throw std::runtime_error("Unable to open file for reading.");
    }

    // Clear existing records
    clear();

    std::string line;

    while (std::getline(file, line))
    {
        std::stringstream ss(line);

        std::string idStr;
        std::string embeddingStr;
        std::string metadata;

        // Split the line
        std::getline(ss, idStr, '|');
        std::getline(ss, embeddingStr, '|');
        std::getline(ss, metadata);

        // Convert ID
        int id = std::stoi(idStr);

        // Parse embedding
        std::vector<float> embedding;
        std::stringstream embeddingStream(embeddingStr);

        std::string value;

        while (std::getline(embeddingStream, value, ','))
        {
            embedding.push_back(std::stof(value));
        }

        // Create record
        VectorRecord record(id, embedding, metadata);

        // Insert into all indexes
        insert(record);
    }

    file.close();
}
// KNN representation with sort

std::vector<SearchResult> VectorDatabase::knnSearch(
    const std::vector<float> &query,
    int k,
    Metric metric,
    const std::string &metadataFilter)
{
    // Stores every record along with its score.
    std::vector<SearchResult> scoredRecords;

    // Calculate score for every record.
    for (const auto &record : records)
    {
        // Skip records that don't match the metadata filter.
        if (!metadataFilter.empty() &&
            record.metadata != metadataFilter)
        {
            continue;
        }

        float score = 0.0f;

        switch (metric)
        {
        case Metric::COSINE:
            score = Similarity::cosineSimilarity(query, record.embedding);
            break;

        case Metric::EUCLIDEAN:
            score = Similarity::euclideanDistance(query, record.embedding);
            break;

        case Metric::DOT_PRODUCT:
            score = Similarity::dotproduct(query, record.embedding);
            break;

        default:
            throw std::invalid_argument("Unsupported metric.");
        }

        scoredRecords.push_back({record, score});
    }

    // Sort according to the selected metric.
    if (metric == Metric::EUCLIDEAN)
    {
        std::sort(
            scoredRecords.begin(),
            scoredRecords.end(),
            [](const SearchResult &a, const SearchResult &b)
            {
                return a.score < b.score;
            });
    }
    else
    {
        std::sort(
            scoredRecords.begin(),
            scoredRecords.end(),
            [](const SearchResult &a, const SearchResult &b)
            {
                return a.score > b.score;
            });
    }

    // Keep only the Top-K results.
    std::vector<SearchResult> result;

    for (int i = 0;
         i < k && i < static_cast<int>(scoredRecords.size());
         i++)
    {
        result.push_back(scoredRecords[i]);
    }

    return result;
}

// KNN search implementation with heap

std::vector<SearchResult> VectorDatabase::knnSearchOptimized(
    const std::vector<float> &query,
    int k,
    Metric metric,
    const std::string &metadataFilter)
{
    // Min Heap:
    // Stores only the Top-K search results.
    // The smallest score is always at the top.
    std::priority_queue<
        SearchResult,
        std::vector<SearchResult>,
        CompareSearchResult>
        heap;

    // Traverse every record in the database.
    for (const auto &record : records)
    {
        // Skip records whose metadata does not match the filter.
        // If metadataFilter is empty, all records are searched.
        if (!metadataFilter.empty() &&
            record.metadata != metadataFilter)
        {
            continue;
        }

        float score = 0.0f;

        // Compute similarity/distance based on the selected metric.
        switch (metric)
        {
        case Metric::COSINE:
            score = Similarity::cosineSimilarity(query, record.embedding);
            break;

        case Metric::EUCLIDEAN:
            score = Similarity::euclideanDistance(query, record.embedding);
            break;

        case Metric::DOT_PRODUCT:
            score = Similarity::dotproduct(query, record.embedding);
            break;

        default:
            throw std::invalid_argument("Unsupported metric.");
        }

        // Create the current search result.
        SearchResult current{record, score};

        // If heap contains fewer than K elements,
        // simply insert the current result.
        if (heap.size() < k)
        {
            heap.push(current);
        }
        else
        {
            // Decide whether the current result is better
            // than the weakest result currently in the heap.

            bool shouldReplace = false;

            if (metric == Metric::EUCLIDEAN)
            {
                // For Euclidean distance:
                // Smaller distance is better.
                shouldReplace = current.score < heap.top().score;
            }
            else
            {
                // For Cosine Similarity and Dot Product:
                // Larger score is better.
                shouldReplace = current.score > heap.top().score;
            }

            // Replace the weakest result if the current one is better.
            if (shouldReplace)
            {
                heap.pop();
                heap.push(current);
            }
        }
    }

    // Stores the final Top-K search results.
    std::vector<SearchResult> result;

    // Extract all elements from the heap.
    // Since it is a min heap, the weakest result comes out first.
    while (!heap.empty())
    {
        result.push_back(heap.top());
        heap.pop();
    }

    // Reverse only for similarity metrics.
    // Heap returns:
    // Smallest -> Largest
    //
    // We want:
    // Largest -> Smallest
    if (metric != Metric::EUCLIDEAN)
    {
        std::reverse(result.begin(), result.end());
    }

    return result;
}

// BenchMark
Benchmark VectorDatabase::benchmark(
    const std::vector<float> &query)
{
    Benchmark result;

    constexpr int ITERATIONS = 1000;

    // ----------------------------
    // Brute Force
    // ----------------------------
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < ITERATIONS; i++)
    {
        search(query, SearchAlgorithm::BRUTE_FORCE);
    }

    auto end = std::chrono::high_resolution_clock::now();

    result.bruteForceTime =
        std::chrono::duration<double, std::micro>(
            end - start)
            .count() / ITERATIONS;

    // ----------------------------
    // KD-Tree
    // ----------------------------
    start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < ITERATIONS; i++)
    {
        search(query, SearchAlgorithm::KD_TREE);
    }

    end = std::chrono::high_resolution_clock::now();

    result.kdTreeTime =
        std::chrono::duration<double, std::micro>(
            end - start)
            .count() / ITERATIONS;

    // ----------------------------
    // LSH
    // ----------------------------
    start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < ITERATIONS; i++)
    {
        search(query, SearchAlgorithm::LSH);
    }

    end = std::chrono::high_resolution_clock::now();

    result.lshTime =
        std::chrono::duration<double, std::micro>(
            end - start)
            .count() / ITERATIONS;

    // ----------------------------
    // HNSW
    // ----------------------------
    start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < ITERATIONS; i++)
    {
        search(query, SearchAlgorithm::HNSW);
    }

    end = std::chrono::high_resolution_clock::now();

    result.hnswTime =
        std::chrono::duration<double, std::micro>(
            end - start)
            .count() / ITERATIONS;

    return result;
}

const std::vector<VectorRecord>& VectorDatabase::getRecords() const
{
    return records;
}
#include "VectorDatabase.h"
#include "Similarity.h"
#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include "SearchResult.h"
#include "Metric.h"

void VectorDatabase::insert(const VectorRecord &record)
{
    records.push_back(record);
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

// Remove a record by its ID.

void VectorDatabase::remove(int id)
{
    records.erase(
        std::remove_if(
            records.begin(),
            records.end(),
            [id](const VectorRecord &record)
            {
                return record.id == id;
            }),
        records.end());
}

VectorRecord VectorDatabase::search(const std::vector<float> &query)
{
    // Check if the database is empty
    if (records.empty())
    {
        throw std::runtime_error("Database is empty.");
    }

    // Assume the first record is the best match initially
    VectorRecord bestRecord = records[0];

    // Calculate the similarity score of the first record
    double bestScore =
        Similarity::cosineSimilarity(query, bestRecord.embedding);

    // Compare the query with all remaining records
    for (size_t i = 1; i < records.size(); i++)
    {
        // Calculate similarity of the current record
        double currentScore =
            Similarity::cosineSimilarity(query, records[i].embedding);

        // If the current record is more similar, update the best match
        if (currentScore > bestScore)
        {
            bestScore = currentScore;
            bestRecord = records[i];
        }
    }

    // Return the most similar record found
    return bestRecord;
}

// save the database to a file sirf read krega
void VectorDatabase::saveToFile(const std::string &filename) const
{
    std::ofstream file(filename);

    //  check if the file is open
    if (!file.is_open())
    {
        throw std::runtime_error("Unable to open file for writing.");
    }

    // loop to store every file
    for (const auto &record : records)
    {
        file << record.id << "|";
        //  for loop for embedding in float
        for (size_t i = 0; i < record.embedding.size(); i++)
        {
            file << record.embedding[i];

            if (i != record.embedding.size() - 1)
            {
                file << ",";
            }
        }
        file << "|" << record.metadata << std::endl;
    }

    file.close();
}

// load the file

void VectorDatabase::loadFromFile(const std::string &filename)
{
    std::ifstream file(filename);

    if (!file.is_open())
    {
        throw std::runtime_error("Unable to open file for reading.");
    }

    records.clear(); // Clear existing records before loading

    std::string line;

    while (std::getline(file, line))
    {
        std::stringstream ss(line);

        std::string idStr;
        std::string embeddingStr;
        std::string metadata;

        // Split the line into three parts
        std::getline(ss, idStr, '|');
        std::getline(ss, embeddingStr, '|');
        std::getline(ss, metadata);

        // Convert ID from string to int
        int id = std::stoi(idStr);

        // Parse embedding
        std::vector<float> embedding;
        std::stringstream embeddingStream(embeddingStr);

        std::string value;

        while (std::getline(embeddingStream, value, ','))
        {
            embedding.push_back(std::stof(value));
        }

        // Create VectorRecord and add to database
        records.emplace_back(id, embedding, metadata);
    }

    file.close();
}

// KNN search implementation

std::vector<SearchResult> VectorDatabase::knnSearch(
    const std::vector<float> &query,
    int k,
    Metric metric)
{
    // Stores each record with its similarity score.
    std::vector<SearchResult> scoredRecords;

    // Compute similarity for every record.
    for (const auto &record : records)
    {
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
            throw std::invalid_argument("Unsupported metric type.");
        }

        scoredRecords.push_back({record, score});
    }

    // Sort by similarity (highest first).
    if (metric == Metric::EUCLIDEAN)
    {
        // Smaller distance is better.
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
        // Larger similarity score is better.
        std::sort(
            scoredRecords.begin(),
            scoredRecords.end(),
            [](const SearchResult &a, const SearchResult &b)
            {
                return a.score > b.score;
            });
    }
    // Stores the final Top-K results.
    std::vector<SearchResult> result;

    // Copy the first k results.
    for (int i = 0; i < k && i < scoredRecords.size(); i++)
    {
        result.push_back(scoredRecords[i]);
    }

    return result;
}
#include "VectorDatabase.h"
#include "Similarity.h"
#include <iostream>
#include <algorithm>
#include <stdexcept>

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

VectorRecord VectorDatabase::search(const std::vector<float>& query)
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
#include "LSH.h"
#include<iostream>
#include "Similarity.h"
#include <algorithm>

using namespace std;

int LSH::hash(const vector<float>& embedding)
{
    // Compute the sum of all dimensions.
    float sum = 0.0f;

    for (float value : embedding)
    {
        sum += value;
    }

    // Size of each bucket.
    const int bucketSize = 10;

    // Return the bucket number. helpt o return the interger 
    return static_cast<int>(sum / bucketSize);
}

void LSH::insert(const VectorRecord& record)
{
    // Compute the bucket number for this vector.
    int bucket = hash(record.embedding);

    // Insert the record into the corresponding bucket.
    buckets[bucket].push_back(record);
}

void LSH::display() const
{
    cout << "\n===== LSH Buckets =====\n";

    // Traverse every bucket in the hash table.
    for (const auto& bucket : buckets)
    {
        cout << "Bucket " << bucket.first << endl;

        // Traverse every record inside the bucket.
        for (const auto& record : bucket.second)
        {
            cout << "  ID: " << record.id
                      << "  Metadata: " << record.metadata
                      << endl;
        }

        cout << "------------------------\n";
    }
}

VectorRecord LSH::search(const vector<float>& query){
    // compute the bucket number for the query vector
    int bucket = hash(query);

    // check the bucket in hashmap

    if(buckets.find(bucket)==buckets.end()){
      throw runtime_error("No records found in the corresponding bucket.");
    } 

    // Get all vectors inside the bucket.
    const auto& records = buckets[bucket];

    VectorRecord bestRecord = records[0];
    float bestDistance =
    Similarity::euclideanDistance(query, bestRecord.embedding);

    for(size_t i =1;i<records.size();i++){
        float distance= Similarity::euclideanDistance(
            query,
            records[i].embedding
        );

        if(distance < bestDistance){
            bestDistance = distance;
            bestRecord = records[i];
        }
    }
    return bestRecord;
}

void LSH::remove(int id)
{
    for (auto it = buckets.begin(); it != buckets.end(); )
    {
        auto &records = it->second;

        for (int i = 0; i < records.size(); i++)
        {
            if (records[i].id == id)
            {
                records.erase(records.begin() + i);
                break;
            }
        }

        if (records.empty())
            it = buckets.erase(it);
        else
            ++it;
    }
}
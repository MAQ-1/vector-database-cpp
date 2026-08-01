#include <iostream>
#include <vector>
#include <algorithm>
#include "VectorDatabase.h"
#include "VectorRecord.h"
#include "Metric.h"
using namespace std;
int main()
{
    VectorDatabase db;

    // Insert sample records
    db.insert(VectorRecord(1, {1.0f, 2.0f, 3.0f}, "Dog"));
    db.insert(VectorRecord(2, {2.0f, 3.0f, 4.0f}, "Cat"));
    db.insert(VectorRecord(3, {1.0f, 1.0f, 1.0f}, "Bird"));
    db.insert(VectorRecord(4, {5.0f, 6.0f, 7.0f}, "Tiger"));
    db.insert(VectorRecord(5, {1.5f, 2.5f, 3.5f}, "Lion"));

    std::cout << "=============================\n";
    std::cout << "All Records in Database\n";
    std::cout << "=============================\n\n";

    db.display();

    // Query vector
   std::vector<float> query = {1.0f, 2.0f, 3.0f};
int k = 3;

std::vector<Metric> metrics =
{
    Metric::COSINE,
    Metric::EUCLIDEAN,
    Metric::DOT_PRODUCT
};

std::vector<std::string> names =
{
    "Cosine Similarity",
    "Euclidean Distance",
    "Dot Product"
};

for (int i = 0; i < metrics.size(); i++)
{
    std::cout << "\n=============================\n";
    std::cout << names[i] << '\n';
    std::cout << "=============================\n\n";

    auto nearest = db.knnSearch(query, k, metrics[i]);

    for (const auto& result : nearest)
    {
        std::cout << "ID: " << result.record.id << '\n';
        std::cout << "Score: " << result.score << '\n';
        std::cout << "Metadata: " << result.record.metadata << "\n\n";
    }
}

    return 0;
}
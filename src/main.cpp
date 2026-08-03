#include <iostream>
#include "HNSW.h"

int main()
{
    HNSW index;

    VectorRecord r1(1, {1.0f, 2.0f}, "A");
    VectorRecord r2(2, {2.0f, 3.0f}, "B");
    VectorRecord r3(3, {8.0f, 8.0f}, "C");
    VectorRecord r4(4, {7.0f, 9.0f}, "D");
    VectorRecord r5(5, {1.5f, 2.5f}, "E");

    index.insert(r1);
    index.insert(r2);
    index.insert(r3);
    index.insert(r4);
    index.insert(r5);

  std::vector<float> query = {100.0f, 100.0f};

    HNSWNode* result =
        index.greedySearch(
            index.getEntryPoint(),
            query,
            0);

    if(result != nullptr)
    {
        std::cout
            << "Nearest Node ID : "
            << result->record.id
            << std::endl;
    }

    return 0;
}
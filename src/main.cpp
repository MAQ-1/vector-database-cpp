#include <iostream>
#include "HNSW.h"

int main()
{
    HNSW graph;

    graph.insert(VectorRecord(1, {1.0f, 2.0f}, "A"));
    graph.insert(VectorRecord(2, {3.0f, 4.0f}, "B"));
    graph.insert(VectorRecord(3, {5.0f, 6.0f}, "C"));

    std::cout << "Nodes inserted successfully!\n";

    return 0;
}
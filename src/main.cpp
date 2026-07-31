#include <iostream>
#include <vector>
#include "Similarity.h"
#include "VectorDatabase.h"

using namespace std;

int main()
{
    VectorDatabase db;

    db.insert(VectorRecord(1, {1.2f, 2.3f, 3.4f}, "First Record"));
    db.insert(VectorRecord(2, {4.5f, 5.6f, 6.7f}, "Second Record"));

    db.saveToFile("database.txt");

    VectorDatabase newDb;

    newDb.loadFromFile("database.txt");

    newDb.display();

    return 0;
}
// {

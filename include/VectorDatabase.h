#ifndef VECTOR_DATABASE_H
#define VECTOR_DATABASE_H

#include <vector>
#include "VectorRecord.h"

class VectorDatabase
{
private:
    std::vector<VectorRecord> records;

public:
    void insert(const VectorRecord& record);

    void remove(int id);

    VectorRecord search(const std::vector<float>& query);
  
    // It means this function promises not to modify the database.
     void display() const;
};

#endif
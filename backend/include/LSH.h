#pragma once

#include <vector>
#include <unordered_map>
#include "VectorRecord.h"

class LSH
{
private:
 std::unordered_map<int, std::vector<VectorRecord>> buckets;

 int hash(const std::vector<float>& embedding ) ;
public:
   
void insert(const VectorRecord& record);
void display() const;

void remove(int id);

VectorRecord search(
    const std::vector<float>& query
);
};
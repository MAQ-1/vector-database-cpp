#ifndef VECTOR_DATABASE_H
#define VECTOR_DATABASE_H

#include <vector>
#include "VectorRecord.h"
#include "SearchResult.h"

class VectorDatabase
{
private:
    std::vector<VectorRecord> records;

public:
    void insert(const VectorRecord& record);

    void remove(int id);

    VectorRecord search(const std::vector<float>& query);
       
    // save the database to a file
    // Saving only reads the database.
    void saveToFile(const std::string& filename) const;


//   load the database from a file
//    Loading changes the database.
    void loadFromFile(const std::string& filename);
  
    // It means this function promises not to modify the database.
     void display() const;

    //  KNN 

    std::vector<SearchResult> knnSearch(
    const std::vector<float>& query,
    int k
  );
};

#endif
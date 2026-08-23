#ifndef VECTOR_RECORD_H
#define VECTOR_RECORD_H

#include <vector>
#include <string>

class VectorRecord {
    public:

     int id;

     std:: vector<float> embedding;
     std:: string metadata;

     VectorRecord(
        int id,
        const std::vector<float>& embedding,
        const std::string& metadata
     );

};

#endif
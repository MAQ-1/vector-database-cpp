#include "VectorRecord.h"

VectorRecord::VectorRecord(
    int id,
    const std::vector<float> &embedding,
    const std::string &metadata)
     : id(id), embedding(embedding), metadata(metadata) {}
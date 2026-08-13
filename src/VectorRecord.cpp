#include "VectorRecord.h"

using namespace std;

VectorRecord::VectorRecord(
    int id,
    const vector<float> &embedding,
    const string &metadata)
     : id(id), embedding(embedding), metadata(metadata) {}
// this file declare the function only so oterh can use it freely and easy to maintan
// not using usingname std in header file 
#ifndef SIMILARITY_H
#define SIMILARITY_H

#include <vector>

class Similarity
{
public:
    static double euclideanDistance(
        const std::vector<float>& a,
        const std::vector<float>& b);

    static double manhattanDistance(
        const std::vector<float>& a,
        const std::vector<float>& b);

    static double cosineSimilarity(
        const std::vector<float>& a,
        const std::vector<float>& b);

    static double dotproduct(
        const std::vector<float>& a,
        const std::vector<float>& b);

    static double magnitude(
        const std::vector<float>& a);
    
};

#endif
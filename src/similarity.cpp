#include "Similarity.h"
#include <cmath>
#include <stdexcept>

using namespace std;

// ======================================================
// Function: Euclidean Distance
// Purpose : Calculate the straight-line distance between
//           two vectors.
// Formula : √((a1-b1)² + (a2-b2)² + ... + (an-bn)²)
// ======================================================
double Similarity::euclideanDistance(
    const vector<float>& a,
    const vector<float>& b)
{
    // Ensure both vectors have the same number of dimensions.
    // Comparing vectors of different sizes is mathematically invalid.
    if (a.size() != b.size())
    {
        throw invalid_argument("Vectors must have the same dimensions.");
    }

    // Stores the sum of squared differences.
    double sum = 0.0;

    // Visit every dimension of the vectors.
    for (size_t i = 0; i < a.size(); i++)
    {
        // Difference between the current dimension.
        double difference = a[i] - b[i];

        // Square the difference and add it to the running total.
        sum += difference * difference;
    }

    // Return the square root of the accumulated value.
    return sqrt(sum);
}

// ======================================================
// Function : Manhattan Distance
// Purpose  : Calculate the total distance by adding the
//            absolute difference of each dimension.
// Formula  : |a1-b1| + |a2-b2| + ... + |an-bn|
// ======================================================
double Similarity::manhattanDistance(
    const vector<float>& a,
    const vector<float>& b)
{
    // Ensure both vectors have the same number of dimensions.
    if (a.size() != b.size())
    {
        throw invalid_argument("Vectors must have the same dimensions.");
    }

    // Stores the total Manhattan distance.
    double sum = 0.0;

    // Visit every dimension.
    for (size_t i = 0; i < a.size(); i++)
    {
        // Add the absolute difference of the current dimension.
        sum += std::abs(a[i] - b[i]);
    }

    // Return the total Manhattan distance.
    return sum;
}


// ======================================================
// Function : Dot Product
// Purpose  : Multiply corresponding elements of two
//            vectors and return their sum.
// Formula  : (a1*b1) + (a2*b2) + ... + (an*bn)
// =

double Similarity::dotproduct( const vector<float> &a , const vector<float> &b ) {
    // Ensure both vectors have the same number of dimensions.
    if (a.size() != b.size()) {
        throw invalid_argument("Vectors must have the same dimensions.");
    }

    // Stores the total dot product.
    double sum = 0.0;

    // Visit every dimension.
    for (size_t i = 0; i < a.size(); i++) {
        // Multiply corresponding elements and add to the total.
        sum += a[i] * b[i];
    }

    // Return the total dot product.
    return sum;
}


// Now time for magnitude 
 double Similarity::magnitude( const vector<float> &a ) {
    // Stores the sum of squares.
    double sum = 0.0;

    // Visit every dimension.
    for (size_t i = 0; i < a.size(); i++) {
        // Square the current element and add to the total.
        sum += a[i] * a[i];
    }

    // Return the square root of the accumulated value.
    return sqrt(sum);
}

 double Similarity::cosineSimilarity(const vector<float> &a, const vector<float> &b) {
     
    if(a.size() != b.size()) {
        throw invalid_argument("Vectors must have the same dimensions.");
    }

    double mgA = magnitude(a);
    double mgB = magnitude(b);
    // prevent division by 0

    if(mgA == 0 || mgB == 0) {
        throw invalid_argument("Magnitude of one or both vectors is zero, cannot compute cosine similarity.");
    }
    // cosine similarity 
     
    return dotproduct(a,b)/(mgA*mgB);
 }
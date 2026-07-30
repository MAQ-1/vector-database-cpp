#include <iostream>
#include <vector>
#include "Similarity.h"
#include "VectorDatabase.h"

using namespace std;

int main()
{
    VectorDatabase db;

    VectorRecord r1(
        1,
        {1.2f, 2.3f, 3.4f},
        "First Record"
    );

    VectorRecord r2(
        2,
        {4.5f, 5.6f, 6.7f},
        "Second Record"
    );

    db.insert(r1);
    db.insert(r2);

    cout << "Records inserted successfully!" << endl;
        db.display();
        db.remove(1);
        cout << "\nAfter removing ID 1:\n";
        db.display();
    return 0;
}
// {
//     vector<float> a = {1, 2, 3};
//     vector<float> b = {1, 2, 3};

//     cout << "Euclidean Distance : "
//          << Similarity::euclideanDistance(a, b) << endl;

//     cout << "Manhattan Distance : "
//          << Similarity::manhattanDistance(a, b) << endl;

//     cout << "Dot Product        : "
//          << Similarity::dotproduct(a, b) << endl;

//     cout << "Magnitude of A     : "
//          << Similarity::magnitude(a) << endl;

//     cout << "Magnitude of B     : "
//          << Similarity::magnitude(b) << endl;

//     cout << "Cosine Similarity  : "
//          << Similarity::cosineSimilarity(a, b) << endl;

//     return 0;
// }
#include "PdfExtractor.h"

#include <iostream>

using namespace std;

int main()
{
    try
    {
        string text =
            PdfExtractor::extractText("test/pdftesting.pdf");

        cout << "PDF extraction successful.\n";
        cout << "Extracted characters: "
             << text.size()
             << "\n\n";

        cout << "========== TEXT ==========\n";
        cout << text;
        cout << "\n===========================\n";
    }
    catch (const exception& e)
    {
        cerr << "ERROR: " << e.what() << endl;
        return 1;
    }

    return 0;
}
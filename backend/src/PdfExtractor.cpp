#include "PdfExtractor.h"

#include <poppler-document.h>
#include <poppler-page.h>

#include <stdexcept>
#include <sstream>

using namespace std;

string PdfExtractor::extractText(const string& filePath)
{
    if (filePath.empty())
    {
        throw runtime_error("PDF file path cannot be empty.");
    }

    poppler::document* document =
        poppler::document::load_from_file(filePath);

    if (!document)
    {
        throw runtime_error(
            "Failed to open PDF: " + filePath
        );
    }

    ostringstream text;

    int pageCount = document->pages();

    for (int i = 0; i < pageCount; i++)
    {
        poppler::page* page = document->create_page(i);

        if (!page)
        {
            continue;
        }

        poppler::ustring pageText = page->text();

       auto utf8 = pageText.to_utf8();
text.write(utf8.data(), utf8.size());

        if (i + 1 < pageCount)
        {
            text << "\n\n";
        }

        delete page;
    }

    delete document;

    string result = text.str();

    if (result.empty())
    {
        throw runtime_error(
            "PDF contains no extractable text."
        );
    }

    return result;
}
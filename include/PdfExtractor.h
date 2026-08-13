#ifndef PDF_EXTRACTOR_H
#define PDF_EXTRACTOR_H

#include <string>

class PdfExtractor
{
public:
    static std::string extractText(const std::string& filePath);
};

#endif
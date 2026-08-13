#ifndef DOCUMENT_INGESTION_H
#define DOCUMENT_INGESTION_H

#include <string>
#include <vector>
#include <unordered_map>

class OllamaClient;
class HNSW;
class VectorRecord;

class DocumentIngestion
{
public:
    DocumentIngestion(
        OllamaClient& ollamaClient,
        HNSW& documentHnsw,
        std::vector<VectorRecord>& documents,
        std::unordered_map<int, std::string>& documentTexts,
        int& nextDocumentId
    );

    int ingest(
        const std::string& title,
        const std::string& text
    );

private:
    OllamaClient& ollamaClient;
    HNSW& documentHnsw;
    std::vector<VectorRecord>& documents;
    std::unordered_map<int, std::string>& documentTexts;
    int& nextDocumentId;
};

#endif
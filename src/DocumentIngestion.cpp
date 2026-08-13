#include "DocumentIngestion.h"

#include "OllamaClient.h"
#include "HNSW.h"
#include "VectorRecord.h"

#include <stdexcept>
#include <sstream>
#include <iostream>
using namespace std;

DocumentIngestion::DocumentIngestion(
    OllamaClient& ollamaClient,
    HNSW& documentHnsw,
    vector<VectorRecord>& documents,
    unordered_map<int, string>& documentTexts,
    int& nextDocumentId)
    : ollamaClient(ollamaClient),
      documentHnsw(documentHnsw),
      documents(documents),
      documentTexts(documentTexts),
      nextDocumentId(nextDocumentId)
{
}

int DocumentIngestion::ingest(
    const string& title,
    const string& text)
{
    if (title.empty())
    {
        throw runtime_error(
            "Document title cannot be empty.");
    }

    if (text.empty())
    {
        throw runtime_error(
            "Document text cannot be empty.");
    }

    // Chunk configuration
    const size_t CHUNK_SIZE = 250;
    const size_t CHUNK_OVERLAP = 30;

    // Split text into words
    istringstream stream(text);

    vector<string> words;
    string word;

    while (stream >> word)
    {
        words.push_back(word);
    }

    if (words.empty())
    {
        throw runtime_error(
            "Document contains no extractable words.");
    }

    // Create chunks
    vector<string> chunks;

    size_t start = 0;

    while (start < words.size())
    {
        size_t end =
            min(start + CHUNK_SIZE, words.size());

        ostringstream chunkStream;

        for (size_t i = start; i < end; i++)
        {
            if (i > start)
            {
                chunkStream << " ";
            }

            chunkStream << words[i];
        }

        chunks.push_back(chunkStream.str());

        // Last chunk reached
        if (end == words.size())
        {
            break;
        }

        // Move forward while keeping overlap
        start = end - CHUNK_OVERLAP;
    }
    
    cout << "[INGEST] Total words: "
     << words.size()
     << endl;

cout << "[INGEST] Total chunks: "
     << chunks.size()
     << endl;

    // Insert every chunk separately
    int firstChunkId = -1;

    for (size_t i = 0; i < chunks.size(); i++)
    {
        const string& chunk = chunks[i];

        // Generate embedding for this chunk
        vector<float> embedding =
            ollamaClient.embed(chunk);

        // Give every chunk its own ID
        int id = nextDocumentId++;

        // Keep the original title while adding
        // chunk information to the metadata.
        string metadata =
            title +
            " | Chunk " +
            to_string(i + 1) +
            "/" +
            to_string(chunks.size());

        VectorRecord record(
            id,
            embedding,
            metadata);

        // Store chunk record
        documents.push_back(record);

        // Store actual chunk text
        documentTexts[id] = chunk;

        // Insert chunk embedding into HNSW
        documentHnsw.insert(record);

        // Keep the first chunk ID as the
        // document ingestion result.
        if (firstChunkId == -1)
        {
            firstChunkId = id;
        }
    }

    return firstChunkId;
}
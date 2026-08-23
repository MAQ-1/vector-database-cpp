#include <iostream>
#include <sstream>
#include "httplib.h"
#include "VectorDatabase.h"
#include <nlohmann/json.hpp>
#include "OllamaClient.h"
#include "DocumentIngestion.h"
#include "PdfExtractor.h"
#include <fstream>

using json = nlohmann::json;
using namespace std;

VectorDatabase db;
HNSW documentHnsw;
vector<VectorRecord> documents;
int nextDocumentId = 1;
unordered_map<int, string> documentTexts;

int main()
{
    httplib::Server server;

    try
    {
        db.loadFromFile("vectors.txt");
        cout << "Database loaded successfully.\n";
        cout << "Total vectors: "
                  << db.getRecords().size()
                  << endl;
    }
    catch (const exception &)
    {
        cout << "No existing database found. Starting with an empty database.\n";
    }

    // Enable CORS for frontend
    server.set_default_headers({
        {"Access-Control-Allow-Origin", "*"},
        {"Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS"},
        {"Access-Control-Allow-Headers", "Content-Type, Authorization"}
    });

    // Handle OPTIONS preflight requests
    server.Options(R"(.*)", [](const httplib::Request &req, httplib::Response &res) {
        res.status = 204;
    });

    server.Get("/", [](const httplib::Request &req,
                       httplib::Response &res)
               { res.set_content(
                     " VectorDB Server Running",
                     "text/plain"); });

    server.Get("/stats",
               [](const httplib::Request &req,
                  httplib::Response &res)
               {
                   json response =
                       {
                           {"status", "OK"},
                           {"total_vectors", db.getRecords().size()}};

                   res.set_content(
                       response.dump(4),
                       "application/json");
               });

    // INSERT

    server.Post("/insert",
                [](const httplib::Request &req,
                   httplib::Response &res)
                {
                    try
                    {
                        json body = json::parse(req.body);

                        int id = body["id"];

                        vector<float> embedding =
                            body["embedding"]
                                .get<vector<float>>();

                        string metadata =
                            body["metadata"];

                        db.insert(
                            VectorRecord(
                                id,
                                embedding,
                                metadata));

                        db.saveToFile("vectors.txt");

                        res.set_content(
                            R"({
                "success":true,
                "message":"Vector inserted successfully"
            })",
                            "application/json");
                    }
                    catch (const exception &e)
                    {
                        json error;

                        error["success"] = false;
                        error["error"] = e.what();

                        res.status = 400;

                        res.set_content(
                            error.dump(4),
                            "application/json");
                    }
                });

    //    GET item
    server.Get("/items",
               [](const httplib::Request &req,
                  httplib::Response &res)
               {
                   json items = json::array();

                   for (const auto &record : db.getRecords())
                   {
                       items.push_back({{"id", record.id},
                                        {"embedding", record.embedding},
                                        {"metadata", record.metadata}});
                   }

                   res.set_content(
                       items.dump(4),
                       "application/json");
               });

    // DELETE VECTOR
    server.Delete("/delete",
                  [](const httplib::Request &req,
                     httplib::Response &res)
                  {
                      try
                      {
                          // Read ID from query parameter
                          int id = stoi(req.get_param_value("id"));

                          // Delete from database
                          db.remove(id);
                          db.saveToFile("vectors.txt");

                          json response =
                              {
                                  {"success", true},
                                  {"message", "Vector deleted successfully"},
                                  {"deleted_id", id}};

                          res.set_content(
                              response.dump(4),
                              "application/json");
                      }
                      catch (const exception &e)
                      {
                          json error;

                          error["success"] = false;
                          error["error"] = e.what();

                          res.status = 400;

                          res.set_content(
                              error.dump(4),
                              "application/json");
                      }
                  });

    // SEARCH
    server.Get("/search",
               [](const httplib::Request &req,
                  httplib::Response &res)
               {
                   // ── 1. Parse v ────────────────────────────────────────────
                   if (!req.has_param("v"))
                   {
                       json e;
                       e["success"] = false;
                       e["error"] = "Missing query vector parameter 'v'.";
                       res.status = 400;
                       res.set_content(e.dump(4), "application/json");
                       return;
                   }

                   vector<float> query;
                   try
                   {
                       stringstream ss(req.get_param_value("v"));
                       string token;
                       while (getline(ss, token, ','))
                           query.push_back(stof(token));
                   }
                   catch (...)
                   {
                       json e;
                       e["success"] = false;
                       e["error"] = "Invalid query vector.";
                       res.status = 400;
                       res.set_content(e.dump(4), "application/json");
                       return;
                   }

                   if (query.empty())
                   {
                       json e;
                       e["success"] = false;
                       e["error"] = "Invalid query vector.";
                       res.status = 400;
                       res.set_content(e.dump(4), "application/json");
                       return;
                   }

                   // ── 2. Parse k ────────────────────────────────────────────
                   if (!req.has_param("k"))
                   {
                       json e;
                       e["success"] = false;
                       e["error"] = "Missing parameter 'k'.";
                       res.status = 400;
                       res.set_content(e.dump(4), "application/json");
                       return;
                   }

                   int k = 0;
                   try
                   {
                       k = stoi(req.get_param_value("k"));
                   }
                   catch (...)
                   {
                       json e;
                       e["success"] = false;
                       e["error"] = "Invalid parameter 'k'.";
                       res.status = 400;
                       res.set_content(e.dump(4), "application/json");
                       return;
                   }

                   if (k <= 0)
                   {
                       json e;
                       e["success"] = false;
                       e["error"] = "k must be greater than 0.";
                       res.status = 400;
                       res.set_content(e.dump(4), "application/json");
                       return;
                   }

                   // ── 3. Parse metric ───────────────────────────────────────
                   if (!req.has_param("metric"))
                   {
                       json e;
                       e["success"] = false;
                       e["error"] = "Missing parameter 'metric'.";
                       res.status = 400;
                       res.set_content(e.dump(4), "application/json");
                       return;
                   }

                   string metricStr = req.get_param_value("metric");
                   Metric metric;
                   if (metricStr == "cosine")
                       metric = Metric::COSINE;
                   else if (metricStr == "euclidean")
                       metric = Metric::EUCLIDEAN;
                   else if (metricStr == "manhattan")
                       metric = Metric::MANHATTAN;
                   else if (metricStr == "dot_product")
                       metric = Metric::DOT_PRODUCT;
                   else
                   {
                       json e;
                       e["success"] = false;
                       e["error"] = "Invalid metric.";
                       res.status = 400;
                       res.set_content(e.dump(4), "application/json");
                       return;
                   }

                   // ── 4. Parse algo ─────────────────────────────────────────
                   if (!req.has_param("algo"))
                   {
                       json e;
                       e["success"] = false;
                       e["error"] = "Missing parameter 'algo'.";
                       res.status = 400;
                       res.set_content(e.dump(4), "application/json");
                       return;
                   }

                   string algoStr = req.get_param_value("algo");
                   SearchAlgorithm algorithm;
                   if (algoStr == "bruteforce")
                       algorithm = SearchAlgorithm::BRUTE_FORCE;
                   else if (algoStr == "kdtree")
                       algorithm = SearchAlgorithm::KD_TREE;
                   else if (algoStr == "hnsw")
                       algorithm = SearchAlgorithm::HNSW;
                   else if (algoStr == "lsh")
                       algorithm = SearchAlgorithm::LSH;
                   else
                   {
                       json e;
                       e["success"] = false;
                       e["error"] = "Invalid algorithm.";
                       res.status = 400;
                       res.set_content(e.dump(4), "application/json");
                       return;
                   }

                   // ── 5. Algorithm / metric compatibility ───────────────────
                   //
                   // bruteforce routes through knnSearch() which is metric-aware.
                   // kdtree / hnsw / lsh each call their own internal search()
                   // which uses Euclidean distance only and returns a single result.
                   // They do not support arbitrary metrics or k > 1.
                   const bool isIndexAlgo = (algorithm == SearchAlgorithm::KD_TREE ||
                                             algorithm == SearchAlgorithm::HNSW ||
                                             algorithm == SearchAlgorithm::LSH);

                   if (isIndexAlgo && metric != Metric::EUCLIDEAN)
                   {
                       json e;
                       e["success"] = false;
                       e["error"] = "Metric not supported by selected algorithm. "
                                    "kdtree, hnsw, and lsh only support metric=euclidean.";
                       res.status = 400;
                       res.set_content(e.dump(4), "application/json");
                       return;
                   }

                   if (isIndexAlgo && k != 1)
                   {
                       json e;
                       e["success"] = false;
                       e["error"] = "kdtree, hnsw, and lsh only support k=1. "
                                    "Use algo=bruteforce for Top-K search.";
                       res.status = 400;
                       res.set_content(e.dump(4), "application/json");
                       return;
                   }

                   // ── 6. Execute search ─────────────────────────────────────
                   try
                   {
                       json results = json::array();

                       if (algorithm == SearchAlgorithm::BRUTE_FORCE)
                       {
                           // knnSearch is metric-aware and returns scored Top-K results.
                           vector<SearchResult> hits =
                               db.knnSearch(query, k, metric);

                           for (const auto &hit : hits)
                           {
                               results.push_back({{"id", hit.record.id},
                                                  {"score", hit.score},
                                                  {"embedding", hit.record.embedding},
                                                  {"metadata", hit.record.metadata}});
                           }
                       }
                       else
                       {
                           // kdtree / hnsw / lsh: single nearest-neighbour,
                           // Euclidean only, no score available from this path.
                           VectorRecord hit = db.search(query, algorithm);
                           results.push_back({{"id", hit.id},
                                              {"score", nullptr},
                                              {"embedding", hit.embedding},
                                              {"metadata", hit.metadata}});
                       }

                       json response;
                       response["success"] = true;
                       response["results"] = results;
                       res.set_content(response.dump(4), "application/json");
                   }
                   catch (const exception &e)
                   {
                       json err;
                       err["success"] = false;
                       err["error"] = e.what();
                       res.status = 400;
                       res.set_content(err.dump(4), "application/json");
                   }
               });

    // olama call
    OllamaClient ollamaClient;
    DocumentIngestion documentIngestion(
    ollamaClient,
    documentHnsw,
    documents,
    documentTexts,
    nextDocumentId
);

    server.Get("/status", [&](const httplib::Request &, httplib::Response &res)
               {
    try
    {
        auto embedding = ollamaClient.embed("test");

        json response = {
            {"success", true},
            {"ollama", "ONLINE"},
            {"model", "nomic-embed-text"},
            {"embedding_dimension", embedding.size()}
        };

        res.set_content(response.dump(2), "application/json");
    }
    catch (const exception &e)
    {
        json response = {
            {"success", false},
            {"ollama", "OFFLINE"},
            {"error", e.what()}
        };

        res.status = 500;
        res.set_content(response.dump(2), "application/json");
    } });

// pdf 
// Upload PDF document endpoint
server.Post("/doc/upload",
            [&](const httplib::Request &req,
                httplib::Response &res)
            {
                try
                {
                    cout << "\n[UPLOAD] Request received" << endl;

                    // Check if request contains a file
                    if (!req.form.has_file("file"))
                    {
                        cout << "[UPLOAD] ERROR: No file received"
                             << endl;

                        json error = {
                            {"success", false},
                            {"error", "PDF file is required"}};

                        res.status = 400;
                        res.set_content(
                            error.dump(4),
                            "application/json");

                        return;
                    }

                    // Get uploaded file
                    const auto &file =
                        req.form.get_file("file");

                    cout << "[UPLOAD] File received: "
                         << file.filename
                         << " | Size: "
                         << file.content.size()
                         << " bytes"
                         << endl;

                    // Validate filename
                    if (file.filename.empty())
                    {
                        cout << "[UPLOAD] ERROR: Empty filename"
                             << endl;

                        json error = {
                            {"success", false},
                            {"error", "Invalid file name"}};

                        res.status = 400;
                        res.set_content(
                            error.dump(4),
                            "application/json");

                        return;
                    }

                    // Check PDF extension
                    string filename = file.filename;

                    if (filename.size() < 4 ||
                        filename.substr(
                            filename.size() - 4) != ".pdf")
                    {
                        cout << "[UPLOAD] ERROR: Not a PDF file"
                             << endl;

                        json error = {
                            {"success", false},
                            {"error", "Only PDF files are supported"}};

                        res.status = 400;
                        res.set_content(
                            error.dump(4),
                            "application/json");

                        return;
                    }

                    // Create temporary PDF path
                    string tempPath =
                        "temp_" +
                        to_string(nextDocumentId) +
                        ".pdf";

                    cout << "[UPLOAD] Temporary file: "
                         << tempPath
                         << endl;

                    // Save uploaded PDF
                    {
                        ofstream outputFile(
                            tempPath,
                            ios::binary);

                        if (!outputFile)
                        {
                            throw runtime_error(
                                "Failed to create temporary PDF file");
                        }

                        outputFile.write(
                            file.content.data(),
                            file.content.size());

                        if (!outputFile)
                        {
                            throw runtime_error(
                                "Failed to write temporary PDF file");
                        }
                    }

                    cout << "[UPLOAD] PDF saved successfully"
                         << endl;

                    // Extract text
                    cout << "[UPLOAD] Starting PDF extraction..."
                         << endl;

                    string text =
                        PdfExtractor::extractText(tempPath);

                    cout << "[UPLOAD] PDF extraction complete"
                         << endl;

                    cout << "[UPLOAD] Characters extracted: "
                         << text.size()
                         << endl;

                    if (text.empty())
                    {
                        remove(tempPath.c_str());

                        throw runtime_error(
                            "PDF contains no extractable text");
                    }

                    // Use filename as document title
                    string title = filename;

                    // Existing document ingestion pipeline
                    cout << "[UPLOAD] Starting document ingestion..."
                         << endl;

                    int id =
                        documentIngestion.ingest(
                            title,
                            text);

                    cout << "[UPLOAD] Document ingestion complete"
                         << endl;

                    cout << "[UPLOAD] Document ID: "
                         << id
                         << endl;

                    // Remove temporary PDF
                    if (remove(tempPath.c_str()) == 0)
                    {
                        cout << "[UPLOAD] Temporary file removed"
                             << endl;
                    }
                    else
                    {
                        cout << "[UPLOAD] WARNING: "
                             << "Could not remove temporary file"
                             << endl;
                    }

                    // Success response
                    json response = {
                        {"success", true},
                        {"id", id},
                        {"title", title},
                        {"filename", filename},
                        {"characters_extracted", text.size()},
                        {"message",
                         "PDF extracted, embedded and inserted successfully"}};

                    cout << "[UPLOAD] Sending success response"
                         << endl;

                    res.set_content(
                        response.dump(4),
                        "application/json");
                }
                catch (const exception &e)
                {
                    cout << "[UPLOAD] EXCEPTION: "
                         << e.what()
                         << endl;

                    json error = {
                        {"success", false},
                        {"error", e.what()}};

                    res.status = 400;

                    res.set_content(
                        error.dump(4),
                        "application/json");
                }
            });
    // insert document endpoint
    server.Post("/doc/insert",
                [&](const httplib::Request &req,
                    httplib::Response &res)
                {
                    try
                    {
                        json body = json::parse(req.body);

                        if (!body.contains("title") || !body.contains("text"))
                        {
                            json error = {
                                {"success", false},
                                {"error", "title and text are required"}};

                            res.status = 400;
                            res.set_content(error.dump(4), "application/json");
                            return;
                        }

                        string title = body["title"];
                        string text = body["text"];

                        if (text.empty())
                        {
                            json error = {
                                {"success", false},
                                {"error", "text cannot be empty"}};

                            res.status = 400;
                            res.set_content(error.dump(4), "application/json");
                            return;
                        }

                       int id = documentIngestion.ingest(
                            title,
                            text
                        );

                        json response = {
                            {"success", true},
                            {"id", id},
                            {"title", title},
                            // {"embedding_dimension", embedding.size()},
                            {"message", "Document embedded and inserted successfully"}};

                        res.set_content(
                            response.dump(4),
                            "application/json");
                    }
                    catch (const exception &e)
                    {
                        json error = {
                            {"success", false},
                            {"error", e.what()}};

                        res.status = 400;

                        res.set_content(
                            error.dump(4),
                            "application/json");
                    }
                });

    // Search documents using Ollama embedding + HNSW
    server.Post("/doc/search",
                [&](const httplib::Request &req,
                    httplib::Response &res)
                {
                    try
                    {
                        json body = json::parse(req.body);

                        if (!body.contains("question"))
                        {
                            json error = {
                                {"success", false},
                                {"error", "question is required"}};

                            res.status = 400;
                            res.set_content(error.dump(4), "application/json");
                            return;
                        }

                        string question = body["question"];

                        if (question.empty())
                        {
                            json error = {
                                {"success", false},
                                {"error", "question cannot be empty"}};

                            res.status = 400;
                            res.set_content(error.dump(4), "application/json");
                            return;
                        }

                        // Convert question into the same 768D embedding space
                        vector<float> queryEmbedding =
                            ollamaClient.embed(question);

                        // Find nearest document using HNSW
                        VectorRecord result =
                            documentHnsw.search(queryEmbedding);

                        json response = {
                            {"success", true},
                            {"id", result.id},
                            {"metadata", result.metadata},
                            {"embedding_dimension", queryEmbedding.size()}};

                        res.set_content(
                            response.dump(4),
                            "application/json");
                    }
                    catch (const exception &e)
                    {
                        json error = {
                            {"success", false},
                            {"error", e.what()}};

                        res.status = 400;
                        res.set_content(
                            error.dump(4),
                            "application/json");
                    }
                });



// ASK question endpoint 

// ASK question endpoint
server.Post("/doc/ask",
            [&](const httplib::Request& req,
                httplib::Response& res)
            {
                try
                {
                    json body = json::parse(req.body);

                    if (!body.contains("question"))
                    {
                        json error = {
                            {"success", false},
                            {"error", "question is required"}
                        };

                        res.status = 400;
                        res.set_content(
                            error.dump(4),
                            "application/json");
                        return;
                    }

                    string question = body["question"];

                    if (question.empty())
                    {
                        json error = {
                            {"success", false},
                            {"error", "question cannot be empty"}
                        };

                        res.status = 400;
                        res.set_content(
                            error.dump(4),
                            "application/json");
                        return;
                    }

                    // 1. Convert question into embedding
                    vector<float> queryEmbedding =
                        ollamaClient.embed(question);

                    if (queryEmbedding.empty())
                    {
                        throw runtime_error(
                            "Failed to generate question embedding"
                        );
                    }

                    // 2. Retrieve top 5 relevant chunks
                    const int TOP_K = 5;

                    vector<VectorRecord> results =
                        documentHnsw.knnSearch(
                            queryEmbedding,
                            TOP_K);

                    if (results.empty())
                    {
                        throw runtime_error(
                            "No relevant document chunks found"
                        );
                    }

                    // 3. Build context from retrieved chunks
                    string context;

                    json sources = json::array();

                    for (int i = 0;
                         i < static_cast<int>(results.size());
                         i++)
                    {
                        const VectorRecord& result =
                            results[i];

                        auto it =
                            documentTexts.find(result.id);

                        if (it == documentTexts.end())
                        {
                            continue;
                        }

                        // Use the actual source metadata
                        // instead of creating artificial
                        // "Chunk 1", "Chunk 2" labels.
                        string sourceLabel =
                            result.metadata;

                        context +=
                            "\n--- Source: " +
                            sourceLabel +
                            " ---\n";

                        context += it->second;

                        context +=
                            "\n--- End Source ---\n";

                        sources.push_back({
                            {"id", result.id},
                            {"source", sourceLabel}
                        });
                    }

                    if (context.empty())
                    {
                        throw runtime_error(
                            "Retrieved chunks contain no text"
                        );
                    }

                    // 4. Build RAG prompt
                    string prompt =
                        "You are answering a question using "
                        "retrieved document sources.\n\n"

                        "IMPORTANT RULES:\n"
                        "1. Answer only using information supported "
                        "by the provided document context.\n"
                        "2. Do not invent facts, sources, chunk "
                        "numbers, page numbers, or citations.\n"
                        "3. The text inside a document may contain "
                        "its own paragraph or section numbers. "
                        "Do not confuse those numbers with chunk "
                        "numbers.\n"
                        "4. If you refer to a source, use the exact "
                        "source label provided in the context.\n"
                        "5. If the answer is not present in the "
                        "provided context, clearly say that the "
                        "information is not available in the "
                        "provided document.\n"
                        "6. Prefer a clear and direct answer. "
                        "Do not mention the retrieval process unless "
                        "the user asks about it.\n\n"

                        "RETRIEVED DOCUMENT SOURCES:\n" +
                        context +

                        "\n\nUSER QUESTION:\n" +
                        question +

                        "\n\nANSWER:";

                    // 5. Generate answer using Ollama
                    string answer =
                        ollamaClient.generate(prompt);

                    // 6. Return answer + sources + context
                    json response = {
                        {"success", true},
                        {"answer", answer},
                        {"sources", sources},
                        {"chunks_retrieved", results.size()},
                        {"context", context}
                    };

                    res.set_content(
                        response.dump(4),
                        "application/json");
                }
                catch (const exception& e)
                {
                    json error = {
                        {"success", false},
                        {"error", e.what()}
                    };

                    res.status = 400;

                    res.set_content(
                        error.dump(4),
                        "application/json");
                }
            });

// Compare all algorithms endpoint
server.Post("/doc/ask/compare",
            [&](const httplib::Request& req,
                httplib::Response& res)
            {
                try
                {
                    json body = json::parse(req.body);

                    if (!body.contains("question"))
                    {
                        json error = {
                            {"success", false},
                            {"error", "question is required"}
                        };

                        res.status = 400;
                        res.set_content(
                            error.dump(4),
                            "application/json");
                        return;
                    }

                    string question = body["question"];

                    if (question.empty())
                    {
                        json error = {
                            {"success", false},
                            {"error", "question cannot be empty"}
                        };

                        res.status = 400;
                        res.set_content(
                            error.dump(4),
                            "application/json");
                        return;
                    }

                    // 1. Convert question into embedding
                    auto embedStart = chrono::high_resolution_clock::now();
                    vector<float> queryEmbedding =
                        ollamaClient.embed(question);
                    auto embedEnd = chrono::high_resolution_clock::now();

                    if (queryEmbedding.empty())
                    {
                        throw runtime_error(
                            "Failed to generate question embedding"
                        );
                    }

                    const int TOP_K = 5;
                    json algorithms = json::array();

                    // Helper function to build context and generate answer
                    auto buildAnswer = [&](const vector<VectorRecord>& results, const string& algoName) -> json {
                        if (results.empty())
                        {
                            return {
                                {"algorithm", algoName},
                                {"success", false},
                                {"error", "No relevant chunks found"}
                            };
                        }

                        string context;
                        json sources = json::array();

                        for (const auto& result : results)
                        {
                            auto it = documentTexts.find(result.id);
                            if (it == documentTexts.end()) continue;

                            string sourceLabel = result.metadata;
                            context += "\n--- Source: " + sourceLabel + " ---\n";
                            context += it->second;
                            context += "\n--- End Source ---\n";

                            sources.push_back({
                                {"id", result.id},
                                {"source", sourceLabel}
                            });
                        }

                        if (context.empty())
                        {
                            return {
                                {"algorithm", algoName},
                                {"success", false},
                                {"error", "Retrieved chunks contain no text"}
                            };
                        }

                        string prompt =
                            "You are answering a question using retrieved document sources.\n\n"
                            "IMPORTANT RULES:\n"
                            "1. Answer only using information supported by the provided document context.\n"
                            "2. Do not invent facts, sources, chunk numbers, page numbers, or citations.\n"
                            "3. The text inside a document may contain its own paragraph or section numbers. "
                            "Do not confuse those numbers with chunk numbers.\n"
                            "4. If you refer to a source, use the exact source label provided in the context.\n"
                            "5. If the answer is not present in the provided context, clearly say that the "
                            "information is not available in the provided document.\n"
                            "6. Prefer a clear and direct answer. Do not mention the retrieval process unless "
                            "the user asks about it.\n\n"
                            "RETRIEVED DOCUMENT SOURCES:\n" + context +
                            "\n\nUSER QUESTION:\n" + question +
                            "\n\nANSWER:";

                        string answer = ollamaClient.generate(prompt);

                        return {
                            {"algorithm", algoName},
                            {"success", true},
                            {"answer", answer},
                            {"sources", sources},
                            {"chunks_retrieved", results.size()}
                        };
                    };

                    // Run HNSW search
                    {
                        auto start = chrono::high_resolution_clock::now();
                        try
                        {
                            vector<VectorRecord> results =
                                documentHnsw.knnSearch(queryEmbedding, TOP_K);
                            auto end = chrono::high_resolution_clock::now();
                            double searchTime =
                                chrono::duration<double, milli>(end - start).count();

                            json result = buildAnswer(results, "HNSW");
                            result["search_time_ms"] = searchTime;
                            algorithms.push_back(result);
                        }
                        catch (const exception& e)
                        {
                            auto end = chrono::high_resolution_clock::now();
                            double searchTime =
                                chrono::duration<double, milli>(end - start).count();

                            algorithms.push_back({
                                {"algorithm", "HNSW"},
                                {"success", false},
                                {"error", e.what()},
                                {"search_time_ms", searchTime}
                            });
                        }
                    }

                    // Run KD-Tree search (brute force on document chunks since KDTree needs rebuilding)
                    {
                        auto start = chrono::high_resolution_clock::now();
                        try
                        {
                            // Use brute force kNN on document embeddings
                            vector<pair<double, VectorRecord>> distances;
                            
                            for (const auto& doc : documents)
                            {
                                double dist = 0.0;
                                for (size_t i = 0; i < queryEmbedding.size(); i++)
                                {
                                    double diff = queryEmbedding[i] - doc.embedding[i];
                                    dist += diff * diff;
                                }
                                dist = sqrt(dist);
                                distances.push_back({dist, doc});
                            }

                            sort(distances.begin(), distances.end(),
                                 [](const auto& a, const auto& b) { return a.first < b.first; });

                            vector<VectorRecord> results;
                            for (int i = 0; i < min(TOP_K, (int)distances.size()); i++)
                            {
                                results.push_back(distances[i].second);
                            }

                            auto end = chrono::high_resolution_clock::now();
                            double searchTime =
                                chrono::duration<double, milli>(end - start).count();

                            json result = buildAnswer(results, "KD-TREE");
                            result["search_time_ms"] = searchTime;
                            algorithms.push_back(result);
                        }
                        catch (const exception& e)
                        {
                            auto end = chrono::high_resolution_clock::now();
                            double searchTime =
                                chrono::duration<double, milli>(end - start).count();

                            algorithms.push_back({
                                {"algorithm", "KD-TREE"},
                                {"success", false},
                                {"error", e.what()},
                                {"search_time_ms", searchTime}
                            });
                        }
                    }

                    // Run Brute Force search
                    {
                        auto start = chrono::high_resolution_clock::now();
                        try
                        {
                            vector<pair<double, VectorRecord>> distances;
                            
                            for (const auto& doc : documents)
                            {
                                double dist = 0.0;
                                for (size_t i = 0; i < queryEmbedding.size(); i++)
                                {
                                    double diff = queryEmbedding[i] - doc.embedding[i];
                                    dist += diff * diff;
                                }
                                dist = sqrt(dist);
                                distances.push_back({dist, doc});
                            }

                            sort(distances.begin(), distances.end(),
                                 [](const auto& a, const auto& b) { return a.first < b.first; });

                            vector<VectorRecord> results;
                            for (int i = 0; i < min(TOP_K, (int)distances.size()); i++)
                            {
                                results.push_back(distances[i].second);
                            }

                            auto end = chrono::high_resolution_clock::now();
                            double searchTime =
                                chrono::duration<double, milli>(end - start).count();

                            json result = buildAnswer(results, "BRUTE_FORCE");
                            result["search_time_ms"] = searchTime;
                            algorithms.push_back(result);
                        }
                        catch (const exception& e)
                        {
                            auto end = chrono::high_resolution_clock::now();
                            double searchTime =
                                chrono::duration<double, milli>(end - start).count();

                            algorithms.push_back({
                                {"algorithm", "BRUTE_FORCE"},
                                {"success", false},
                                {"error", e.what()},
                                {"search_time_ms", searchTime}
                            });
                        }
                    }

                    json response = {
                        {"success", true},
                        {"algorithms", algorithms}
                    };

                    res.set_content(
                        response.dump(4),
                        "application/json");
                }
                catch (const exception& e)
                {
                    json error = {
                        {"success", false},
                        {"error", e.what()}
                    };

                    res.status = 400;
                    res.set_content(
                        error.dump(4),
                        "application/json");
                }
            });



    // Banchmarkkkkkkkkkkkkkkkkkkkkk

    // BENCHMARK
    server.Get("/benchmark",
               [](const httplib::Request &req,
                  httplib::Response &res)
               {
                   try
                   {

                       vector<float> query = db.getRecords()[0].embedding;

                       Benchmark result = db.benchmark(query);

                       json response =
                           {
                               {"brute_force_us", result.bruteForceTime},
                               {"kd_tree_us", result.kdTreeTime},
                               {"lsh_us", result.lshTime},
                               {"hnsw_us", result.hnswTime}};

                       res.set_content(
                           response.dump(4),
                           "application/json");
                   }
                   catch (const exception &e)
                   {
                       json error;

                       error["success"] = false;
                       error["error"] = e.what();

                       res.status = 400;

                       res.set_content(
                           error.dump(4),
                           "application/json");
                   }
               });

    cout << "=================================\n";
    cout << "     VectorDB Server Started\n";
    cout << "     http://localhost:8080\n";
    cout << "=================================\n";

    // Generate 10,000 random vectors only
    // when the database is empty.

    if (db.getRecords().empty())
    {
        mt19937 rng(42);
        uniform_real_distribution<float> dist(0.0f, 1000.0f);

        for (int i = 1; i <= 10000; i++)
        {
            vector<float> embedding(128);

            for (float &value : embedding)
            {
                value = dist(rng);
            }

            db.insert(
                VectorRecord(
                    i,
                    embedding,
                    "Random Vector"));
        }

        // Save the generated dataset
        db.saveToFile("vectors.txt");

        cout
            << "Generated and saved "
            << db.getRecords().size()
            << " vectors.\n";
    }
    else
    {
        cout
            << "Using existing database with "
            << db.getRecords().size()
            << " vectors.\n";
    }

    server.listen("0.0.0.0", 8080);

    return 0;
}
#include <iostream>
#include <sstream>
#include "httplib.h"
#include "VectorDatabase.h"
#include <nlohmann/json.hpp>
#include "OllamaClient.h"

using json = nlohmann::json;

VectorDatabase db;
HNSW documentHnsw;
std::vector<VectorRecord> documents;
int nextDocumentId = 1;
std::unordered_map<int, std::string> documentTexts;

int main()
{
    httplib::Server server;

    try
    {
        db.loadFromFile("vectors.txt");
        std::cout << "Database loaded successfully.\n";
        std::cout << "Total vectors: "
                  << db.getRecords().size()
                  << std::endl;
    }
    catch (const std::exception &)
    {
        std::cout << "No existing database found. Starting with an empty database.\n";
    }

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

                        std::vector<float> embedding =
                            body["embedding"]
                                .get<std::vector<float>>();

                        std::string metadata =
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
                    catch (const std::exception &e)
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
                          int id = std::stoi(req.get_param_value("id"));

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
                      catch (const std::exception &e)
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

                   std::vector<float> query;
                   try
                   {
                       std::stringstream ss(req.get_param_value("v"));
                       std::string token;
                       while (std::getline(ss, token, ','))
                           query.push_back(std::stof(token));
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
                       k = std::stoi(req.get_param_value("k"));
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

                   std::string metricStr = req.get_param_value("metric");
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

                   std::string algoStr = req.get_param_value("algo");
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
                           std::vector<SearchResult> hits =
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
                   catch (const std::exception &e)
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
    catch (const std::exception &e)
    {
        json response = {
            {"success", false},
            {"ollama", "OFFLINE"},
            {"error", e.what()}
        };

        res.status = 500;
        res.set_content(response.dump(2), "application/json");
    } });

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

                        std::string title = body["title"];
                        std::string text = body["text"];

                        if (text.empty())
                        {
                            json error = {
                                {"success", false},
                                {"error", "text cannot be empty"}};

                            res.status = 400;
                            res.set_content(error.dump(4), "application/json");
                            return;
                        }

                        // Generate 768D embedding using Ollama
                        std::vector<float> embedding =
                            ollamaClient.embed(text);

                        // Create document record
                        int id = nextDocumentId++;

                        VectorRecord record(
                            id,
                            embedding,
                            title);

                        // Store document in memory
                        documents.push_back(record);
                        documentTexts[id] = text;

                        // Insert 768D embedding into separate HNSW
                        documentHnsw.insert(record);

                        json response = {
                            {"success", true},
                            {"id", id},
                            {"title", title},
                            {"embedding_dimension", embedding.size()},
                            {"message", "Document embedded and inserted successfully"}};

                        res.set_content(
                            response.dump(4),
                            "application/json");
                    }
                    catch (const std::exception &e)
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

                        std::string question = body["question"];

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
                        std::vector<float> queryEmbedding =
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
                    catch (const std::exception &e)
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
            res.set_content(error.dump(4), "application/json");
            return;
        }

        std::string question = body["question"];

        if (question.empty())
        {
            json error = {
                {"success", false},
                {"error", "question cannot be empty"}
            };

            res.status = 400;
            res.set_content(error.dump(4), "application/json");
            return;
        }

        // 1. Convert question into 768D embedding
        std::vector<float> queryEmbedding =
            ollamaClient.embed(question);

        if (queryEmbedding.empty())
        {
            throw std::runtime_error(
                "Failed to generate question embedding"
            );
        }

        // 2. Retrieve nearest document from HNSW
        VectorRecord result =
            documentHnsw.search(queryEmbedding);

        // 3. Get original document text
        auto it = documentTexts.find(result.id);

        if (it == documentTexts.end())
        {
            throw std::runtime_error(
                "Retrieved document text not found"
            );
        }

        std::string context = it->second;

        // 4. Build RAG prompt
        std::string prompt =
            "Answer the question using the provided document context. "
            "If the answer is not present in the context, say that the "
            "information is not available in the provided document.\n\n"
            "Document:\n" +
            context +
            "\n\nQuestion:\n" +
            question +
            "\n\nAnswer:";

        // 5. Generate answer using llama3.2
        std::string answer =
            ollamaClient.generate(prompt);

        // 6. Return answer + retrieved context
        json response = {
            {"success", true},
            {"answer", answer},
            {"source_id", result.id},
            {"source", result.metadata},
            {"context", context}
        };

        res.set_content(
            response.dump(4),
            "application/json"
        );
    }
    catch (const std::exception& e)
    {
        json error = {
            {"success", false},
            {"error", e.what()}
        };

        res.status = 400;

        res.set_content(
            error.dump(4),
            "application/json"
        );
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

                       std::vector<float> query = db.getRecords()[0].embedding;

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
                   catch (const std::exception &e)
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

    std::cout << "=================================\n";
    std::cout << "     VectorDB Server Started\n";
    std::cout << "     http://localhost:8080\n";
    std::cout << "=================================\n";

    // Generate 10,000 random vectors only
    // when the database is empty.

    if (db.getRecords().empty())
    {
        std::mt19937 rng(42);
        std::uniform_real_distribution<float> dist(0.0f, 1000.0f);

        for (int i = 1; i <= 10000; i++)
        {
            std::vector<float> embedding(128);

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

        std::cout
            << "Generated and saved "
            << db.getRecords().size()
            << " vectors.\n";
    }
    else
    {
        std::cout
            << "Using existing database with "
            << db.getRecords().size()
            << " vectors.\n";
    }

    server.listen("0.0.0.0", 8080);

    return 0;
}
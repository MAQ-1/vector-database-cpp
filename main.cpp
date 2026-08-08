#include <iostream>
#include "httplib.h"
#include "VectorDatabase.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

VectorDatabase db;
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

    // algo select

    server.Get("/search",
               [](const httplib::Request &req,
                  httplib::Response &res)
               {
                   try
                   {
                       // Read query vector
                       float x = std::stof(req.get_param_value("x"));
                       float y = std::stof(req.get_param_value("y"));

                       std::vector<float> query = {x, y};

                       // Read algorithm
                       std::string algo =
                           req.get_param_value("algorithm");

                       SearchAlgorithm algorithm;

                       if (algo == "bruteforce")
                           algorithm = SearchAlgorithm::BRUTE_FORCE;

                       else if (algo == "kdtree")
                           algorithm = SearchAlgorithm::KD_TREE;

                       else if (algo == "lsh")
                           algorithm = SearchAlgorithm::LSH;

                       else if (algo == "hnsw")
                           algorithm = SearchAlgorithm::HNSW;

                       else
                           throw std::runtime_error("Invalid algorithm.");

                       VectorRecord result =
                           db.search(query, algorithm);

                       json response =
                           {
                               {"id", result.id},
                               {"embedding", result.embedding},
                               {"metadata", result.metadata}};

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



// Banchmarkkkkkkkkkkkkkkkkkkkkk



// BENCHMARK
server.Get("/benchmark",
[](const httplib::Request& req,
   httplib::Response& res)
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
            {"hnsw_us", result.hnswTime}
        };

        res.set_content(
            response.dump(4),
            "application/json");
    }
    catch(const std::exception& e)
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
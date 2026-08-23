#include "OllamaClient.h"

#include "../external/httplib.h"
#include "../external/nlohmann/json.hpp"

#include <stdexcept>

using json = nlohmann::json;
using namespace std;

vector<float> OllamaClient::embed(const string& text)
{
    if (text.empty())
    {
        throw runtime_error("Text cannot be empty.");
    }

    httplib::Client client("http://localhost:11434");

    json request = {
        {"model", "nomic-embed-text"},
        {"prompt", text}
    };

    auto response = client.Post(
        "/api/embeddings",
        request.dump(),
        "application/json"
    );

    if (!response)
    {
        throw runtime_error("Could not connect to Ollama.");
    }

    if (response->status != 200)
    {
        throw runtime_error(
            "Ollama returned HTTP status: " +
            to_string(response->status)
        );
    }

    json result = json::parse(response->body);

    if (!result.contains("embedding"))
    {
        throw runtime_error(
            "Ollama response does not contain embedding."
        );
    }

    vector<float> embedding =
        result["embedding"].get<vector<float>>();

    if (embedding.size() != 768)
    {
        throw runtime_error(
            "Expected 768-dimensional embedding, got " +
            to_string(embedding.size())
        );
    }

    return embedding;
}

string OllamaClient::generate(const string& prompt)
{
    if (prompt.empty())
    {
        throw runtime_error("Prompt cannot be empty.");
    }

    httplib::Client client("http://localhost:11434");

    json request = {
        {"model", "llama3.2"},
        {"prompt", prompt},
        {"stream", false}
    };

    auto response = client.Post(
        "/api/generate",
        request.dump(),
        "application/json"
    );

    if (!response)
    {
        throw runtime_error("Could not connect to Ollama.");
    }

    if (response->status != 200)
    {
        throw runtime_error(
            "Ollama returned HTTP status: " +
            to_string(response->status)
        );
    }

    json result = json::parse(response->body);

    if (!result.contains("response"))
    {
        throw runtime_error(
            "Ollama response does not contain generated response."
        );
    }

    return result["response"].get<string>();
}
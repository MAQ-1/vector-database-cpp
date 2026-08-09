#include "OllamaClient.h"

#include "../external/httplib.h"
#include "../external/nlohmann/json.hpp"

#include <stdexcept>

using json = nlohmann::json;

std::vector<float> OllamaClient::embed(const std::string& text)
{
    if (text.empty())
    {
        throw std::runtime_error("Text cannot be empty.");
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
        throw std::runtime_error("Could not connect to Ollama.");
    }

    if (response->status != 200)
    {
        throw std::runtime_error(
            "Ollama returned HTTP status: " +
            std::to_string(response->status)
        );
    }

    json result = json::parse(response->body);

    if (!result.contains("embedding"))
    {
        throw std::runtime_error(
            "Ollama response does not contain embedding."
        );
    }

    std::vector<float> embedding =
        result["embedding"].get<std::vector<float>>();

    if (embedding.size() != 768)
    {
        throw std::runtime_error(
            "Expected 768-dimensional embedding, got " +
            std::to_string(embedding.size())
        );
    }

    return embedding;
}
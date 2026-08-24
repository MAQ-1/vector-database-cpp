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

    const char* apiKey = getenv("AICREDITS_API_KEY");
    const char* baseUrl = getenv("AICREDITS_BASE_URL");
    const char* embeddingModel = getenv("AICREDITS_EMBEDDING_MODEL");

    if (!apiKey || string(apiKey).empty())
    {
        throw runtime_error(
            "AICREDITS_API_KEY environment variable is not set."
        );
    }

    string baseUrlStr = (baseUrl && string(baseUrl).length() > 0) 
        ? string(baseUrl) 
        : "api.aicredits.in";

    string embeddingModelStr = (embeddingModel && string(embeddingModel).length() > 0)
        ? string(embeddingModel)
        : "text-embedding-3-small";

    httplib::SSLClient client(baseUrlStr.c_str(), 443);

    client.set_connection_timeout(10, 0);
    client.set_read_timeout(60, 0);

    client.set_default_headers({
        {"Authorization", string("Bearer ") + apiKey}
    });

    json request = {
        {"model", embeddingModelStr},
        {"input", text}
    };

    auto response = client.Post(
        "/v1/embeddings",
        request.dump(),
        "application/json"
    );

    if (!response)
    {
        throw runtime_error(
            "Could not connect to AiCredits embedding API."
        );
    }

    if (response->status != 200)
    {
        throw runtime_error(
            "AiCredits returned HTTP status: " +
            to_string(response->status) +
            "\nResponse: " + response->body
        );
    }

    json result = json::parse(response->body);

    if (!result.contains("data") ||
        !result["data"].is_array() ||
        result["data"].empty() ||
        !result["data"][0].contains("embedding"))
    {
        throw runtime_error(
            "AiCredits response does not contain embedding."
        );
    }

    vector<float> embedding =
        result["data"][0]["embedding"].get<vector<float>>();

    return embedding;
}

string OllamaClient::generate(const string& prompt)
{
    if (prompt.empty())
    {
        throw runtime_error("Prompt cannot be empty.");
    }

    const char* apiKey = getenv("AICREDITS_API_KEY");
    const char* baseUrl = getenv("AICREDITS_BASE_URL");
    const char* model = getenv("AICREDITS_MODEL");

    if (!apiKey || string(apiKey).empty())
    {
        throw runtime_error(
            "AICREDITS_API_KEY environment variable is not set."
        );
    }

    string baseUrlStr = (baseUrl && string(baseUrl).length() > 0) 
        ? string(baseUrl) 
        : "api.aicredits.in";

    string modelStr = (model && string(model).length() > 0)
        ? string(model)
        : "gpt-4o-mini";

    httplib::SSLClient client(baseUrlStr.c_str(), 443);

    client.set_connection_timeout(10, 0);
    client.set_read_timeout(60, 0);

    client.set_default_headers({
        {"Authorization", string("Bearer ") + apiKey}
    });

    json request = {
        {"model", modelStr},
        {"messages", json::array({
            {{"role", "user"}, {"content", prompt}}
        })},
        {"max_tokens", 1000},
        {"temperature", 0.7}
    };

    auto response = client.Post(
        "/v1/chat/completions",
        request.dump(),
        "application/json"
    );

    if (!response)
    {
        throw runtime_error("Could not connect to AiCredits API.");
    }

    if (response->status != 200)
    {
        throw runtime_error(
            "AiCredits API returned HTTP status: " +
            to_string(response->status) +
            "\nResponse: " + response->body
        );
    }

    json result = json::parse(response->body);

    if (!result.contains("choices") ||
        !result["choices"].is_array() ||
        result["choices"].empty() ||
        !result["choices"][0].contains("message") ||
        !result["choices"][0]["message"].contains("content"))
    {
        throw runtime_error(
            "AiCredits API response does not contain generated text."
        );
    }

    return result["choices"][0]["message"]["content"].get<string>();
}
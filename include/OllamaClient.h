#ifndef OLLAMA_CLIENT_H
#define OLLAMA_CLIENT_H

#include <string>
#include <vector>

class OllamaClient
{
public:
    std::vector<float> embed(const std::string& text);
};

#endif
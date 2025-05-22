// LLMInterface.h
#pragma once

#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <atomic>

// �������� API llama.cpp
#include <llama.h>

class LLMInterface {
public:
    // ����������� ��������� ���� � ������
    LLMInterface(const std::string& modelPath);
    ~LLMInterface();

    // ��������� ��������� ��� ��������
    void setContext(const std::string& context);

    // ��������� ������ �� ������
    std::string generateResponse(
        const std::string& prompt,
        bool streamOutput = false,
        std::function<void(const std::string&)> streamCallback = nullptr
    );

    // ��������� ���������
    void stopGeneration();

    // ����� ���������
    void resetContext();

    // ���������� � ������
    std::string getModelInfo() const;

    // ��������, ��������� �� ������
    bool isLoaded() const;

private:
    // ���������� llama.cpp
    llama_model* model;
    llama_context* ctx;
    llama_sampler* sampler;

    // ��������� ��������� (��� ������� API)
    llama_context_params ctxParams;

    // ����������� ������
    std::string contextData;

    // ������� ��� ������������� �������
    mutable std::mutex mtx;

    // ���� ��� ��������� ���������
    std::atomic<bool> stopRequested;

    // ���� �������� ��������
    std::atomic<bool> loaded;

    // ������������� ������
    void initializeModel(const std::string& modelPath);

    // ������������� ��������
    void initializeSampler();

    // ����������� ������
    std::vector<llama_token> tokenize(const std::string& text, bool addBos = false);

    // ������������� �������
    std::string detokenize(const std::vector<llama_token>& tokens);

    // ������� ������������� �������
    llama_token sampleToken(const float* logits, int n_vocab);

    // ������� ��������
    void cleanup();
};
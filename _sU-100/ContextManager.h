// ContextManager.h
#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <algorithm>
#include <numeric>

// ��������� ��� �������� ���������
struct Document {
    std::string name;                    // ��� �����
    std::string content;                 // ������ ����������
    std::vector<std::string> chunks;     // �������� �� ����� �����
    size_t originalSize;                 // ������ ������������� �����
    std::time_t addedTime;               // ����� ����������
};

// ��������� ��� �������������� �����
struct RankedChunk {
    std::string content;
    std::string source;
    float relevanceScore;
    size_t chunkIndex;

    bool operator>(const RankedChunk& other) const {
        return relevanceScore > other.relevanceScore;
    }
};

class ContextManager {
public:
    // ����������� � �������������� �����������
    explicit ContextManager(size_t maxContextTokens = 3000, size_t maxChunkSize = 800);

    // ���������� ��������� � ��������
    void addDocument(const std::string& docName, const std::string& content);

    // ��������� ��������� ��� �������
    std::string getContextForQuery(const std::string& query);

    // ��������� ������ ���� ����������
    std::vector<std::string> getDocumentNames() const;

    // ��������� ���������� ����������
    std::string getDocumentStats() const;

    // ������� ���� ����������
    void clearDocuments();

    // �������� ����������� ���������
    bool removeDocument(const std::string& docName);

    // ��������� ����������
    void setMaxContextTokens(size_t tokens);
    void setMaxChunkSize(size_t size);

private:
    // ���������
    size_t maxContextTokens;
    size_t maxChunkSize;

    // ��������� ����������
    std::map<std::string, std::shared_ptr<Document>> documents;

    // ������� ��� ������������������
    mutable std::mutex mtx;

    // ��������� ����������� �� �����
    std::vector<std::string> chunkContent(const std::string& content);

    // ������������ ������ �� �������������
    std::vector<RankedChunk> rankChunksByRelevance(const std::string& query);

    // ������ ������������� ����� � �������
    float calculateRelevance(const std::string& query, const std::string& chunk);

    // �������� �������� ���������� ������ (TF-IDF ��������)
    std::map<std::string, float> createWordFrequencyMap(const std::string& text);

    // ���������� ����������� ��������
    float cosineSimilarity(const std::map<std::string, float>& vec1,
        const std::map<std::string, float>& vec2);

    // ������ ���������� ������� (��������)
    size_t estimateTokenCount(const std::string& text);

    // ������������ ������ ��� �������
    std::string normalizeText(const std::string& text);

    // ���������� �������� ���� �� �������
    std::vector<std::string> extractKeywords(const std::string& query);
};

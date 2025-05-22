// ContextManager.cpp
#include "ContextManager.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <unordered_set>
#include <cmath>
#include <ctime>
#include <iomanip>
#pragma warning(disable:4996)

ContextManager::ContextManager(size_t maxContextTokens, size_t maxChunkSize)
    : maxContextTokens(maxContextTokens), maxChunkSize(maxChunkSize) {
    std::cout << "ContextManager initialized: max " << maxContextTokens
        << " tokens, chunk size " << maxChunkSize << " chars" << std::endl;
}

void ContextManager::addDocument(const std::string& docName, const std::string& content) {
    std::lock_guard<std::mutex> lock(mtx);

    if (content.empty()) {
        std::cout << "Warning: Empty content for document " << docName << std::endl;
        return;
    }

    auto doc = std::make_shared<Document>();
    doc->name = docName;
    doc->content = content;
    doc->originalSize = content.size();
    doc->addedTime = std::time(nullptr);
    doc->chunks = chunkContent(content);

    documents[docName] = doc;

    std::cout << "✓ Added document '" << docName << "': "
        << content.length() << " chars, "
        << doc->chunks.size() << " chunks" << std::endl;
}

std::string ContextManager::getContextForQuery(const std::string& query) {
    std::lock_guard<std::mutex> lock(mtx);

    if (documents.empty()) {
        return "";
    }

    if (query.empty()) {
        std::cout << "Warning: Empty query provided" << std::endl;
        return "";
    }

    std::cout << "Building context for query: \"" << query.substr(0, 50)
        << (query.length() > 50 ? "..." : "") << "\"" << std::endl;

    // Получаем ранжированные чанки
    auto rankedChunks = rankChunksByRelevance(query);

    if (rankedChunks.empty()) {
        std::cout << "No relevant chunks found" << std::endl;
        return "";
    }

    // Формируем контекст
    std::stringstream contextStream;
    size_t totalTokens = 0;
    size_t selectedChunks = 0;

    contextStream << "=== CONTEXT INFORMATION ===\n\n";

    // Добавляем наиболее релевантные чанки
    for (const auto& chunk : rankedChunks) {
        size_t chunkTokens = estimateTokenCount(chunk.content);

        if (totalTokens + chunkTokens > maxContextTokens) {
            if (selectedChunks == 0) {
                // Если даже первый чанк не помещается, берем его частично
                std::string truncated = chunk.content.substr(0, maxContextTokens * 4); // ~4 символа на токен
                contextStream << "Document: " << chunk.source << " (relevance: "
                    << std::fixed << std::setprecision(2) << chunk.relevanceScore << ")\n";
                contextStream << truncated << "...\n\n";
                selectedChunks++;
            }
            break;
        }

        contextStream << "Document: " << chunk.source << " (relevance: "
            << std::fixed << std::setprecision(2) << chunk.relevanceScore << ")\n";
        contextStream << chunk.content << "\n\n";

        totalTokens += chunkTokens;
        selectedChunks++;
    }

    std::cout << "Context built: " << selectedChunks << " chunks, ~"
        << totalTokens << " tokens" << std::endl;

    return contextStream.str();
}

std::vector<std::string> ContextManager::getDocumentNames() const {
    std::lock_guard<std::mutex> lock(mtx);

    std::vector<std::string> names;
    names.reserve(documents.size());

    for (const auto& [name, doc] : documents) {
        names.push_back(name);
    }

    return names;
}

std::string ContextManager::getDocumentStats() const {
    std::lock_guard<std::mutex> lock(mtx);

    if (documents.empty()) {
        return "No documents loaded";
    }

    std::stringstream ss;
    ss << "=== DOCUMENT STATISTICS ===\n";
    ss << "Total documents: " << documents.size() << "\n\n";

    size_t totalSize = 0;
    size_t totalChunks = 0;

    for (const auto& [name, doc] : documents) {
        totalSize += doc->originalSize;
        totalChunks += doc->chunks.size();

        // Форматируем время добавления
        std::tm* timeInfo = std::localtime(&doc->addedTime);
        char timeBuffer[100];
        std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", timeInfo);

        ss << "📄 " << name << "\n";
        ss << "   Size: " << doc->originalSize << " chars\n";
        ss << "   Chunks: " << doc->chunks.size() << "\n";
        ss << "   Added: " << timeBuffer << "\n\n";
    }

    ss << "Total content: " << totalSize << " characters\n";
    ss << "Total chunks: " << totalChunks << "\n";
    ss << "Estimated tokens: ~" << (totalSize / 4) << "\n";

    return ss.str();
}

void ContextManager::clearDocuments() {
    std::lock_guard<std::mutex> lock(mtx);
    documents.clear();
    std::cout << "✓ All documents cleared from context" << std::endl;
}

bool ContextManager::removeDocument(const std::string& docName) {
    std::lock_guard<std::mutex> lock(mtx);

    auto it = documents.find(docName);
    if (it != documents.end()) {
        documents.erase(it);
        std::cout << "✓ Document '" << docName << "' removed from context" << std::endl;
        return true;
    }

    std::cout << "✗ Document '" << docName << "' not found in context" << std::endl;
    return false;
}

void ContextManager::setMaxContextTokens(size_t tokens) {
    std::lock_guard<std::mutex> lock(mtx);
    maxContextTokens = tokens;
    std::cout << "Max context tokens set to: " << tokens << std::endl;
}

void ContextManager::setMaxChunkSize(size_t size) {
    std::lock_guard<std::mutex> lock(mtx);
    maxChunkSize = size;
    std::cout << "Max chunk size set to: " << size << " characters" << std::endl;
}

std::vector<std::string> ContextManager::chunkContent(const std::string& content) {
    std::vector<std::string> chunks;

    // Разбиваем на предложения и абзацы
    std::istringstream stream(content);
    std::string line;
    std::stringstream currentChunk;
    size_t currentSize = 0;

    while (std::getline(stream, line)) {
        // Если строка пустая, это потенциальная граница чанка
        if (line.empty()) {
            if (currentSize > maxChunkSize / 2) { // Если чанк достаточно большой
                chunks.push_back(currentChunk.str());
                currentChunk.str("");
                currentChunk.clear();
                currentSize = 0;
                continue;
            }
        }

        // Добавляем строку к текущему чанку
        size_t lineSize = line.size() + 1; // +1 для \n

        if (currentSize + lineSize > maxChunkSize && currentSize > 0) {
            // Текущий чанк переполнен, сохраняем его
            chunks.push_back(currentChunk.str());
            currentChunk.str("");
            currentChunk.clear();
            currentSize = 0;
        }

        currentChunk << line << "\n";
        currentSize += lineSize;
    }

    // Добавляем последний чанк
    if (currentSize > 0) {
        chunks.push_back(currentChunk.str());
    }

    // Удаляем слишком короткие чанки
    chunks.erase(
        std::remove_if(chunks.begin(), chunks.end(),
            [](const std::string& chunk) { return chunk.length() < 50; }),
        chunks.end()
    );

    return chunks;
}

std::vector<RankedChunk> ContextManager::rankChunksByRelevance(const std::string& query) {
    std::vector<RankedChunk> rankedChunks;

    // Создаем карту частот для запроса
    auto queryFreq = createWordFrequencyMap(normalizeText(query));

    // Проходим по всем документам и чанкам
    for (const auto& [docName, doc] : documents) {
        for (size_t i = 0; i < doc->chunks.size(); ++i) {
            const auto& chunk = doc->chunks[i];

            // Вычисляем релевантность
            float relevance = calculateRelevance(query, chunk);

            if (relevance > 0.01f) { // Фильтруем совсем нерелевантные чанки
                RankedChunk rankedChunk;
                rankedChunk.content = chunk;
                rankedChunk.source = docName;
                rankedChunk.relevanceScore = relevance;
                rankedChunk.chunkIndex = i;

                rankedChunks.push_back(rankedChunk);
            }
        }
    }

    // Сортируем по релевантности
    std::sort(rankedChunks.begin(), rankedChunks.end(), std::greater<RankedChunk>());

    std::cout << "Ranked " << rankedChunks.size() << " relevant chunks" << std::endl;

    return rankedChunks;
}

float ContextManager::calculateRelevance(const std::string& query, const std::string& chunk) {
    // Нормализуем тексты
    std::string normalizedQuery = normalizeText(query);
    std::string normalizedChunk = normalizeText(chunk);

    // Создаем карты частот
    auto queryFreq = createWordFrequencyMap(normalizedQuery);
    auto chunkFreq = createWordFrequencyMap(normalizedChunk);

    // Вычисляем косинусное сходство
    float similarity = cosineSimilarity(queryFreq, chunkFreq);

    // Дополнительные факторы релевантности

    // 1. Прямое вхождение фраз из запроса
    float directMatch = 0.0f;
    auto keywords = extractKeywords(normalizedQuery);
    for (const auto& keyword : keywords) {
        if (normalizedChunk.find(keyword) != std::string::npos) {
            directMatch += 0.2f; // Бонус за каждое найденное ключевое слово
        }
    }

    // 2. Учитываем длину чанка (предпочитаем средние по размеру чанки)
    float lengthFactor = 1.0f;
    if (chunk.length() < 100) {
        lengthFactor = 0.5f; // Штраф за слишком короткие чанки
    }
    else if (chunk.length() > 2000) {
        lengthFactor = 0.8f; // Небольшой штраф за очень длинные чанки
    }

    // Итоговая релевантность
    return (similarity * 0.7f + directMatch * 0.3f) * lengthFactor;
}

std::map<std::string, float> ContextManager::createWordFrequencyMap(const std::string& text) {
    std::map<std::string, float> freq;
    std::istringstream stream(text);
    std::string word;
    int totalWords = 0;

    while (stream >> word) {
        if (word.length() > 2) { // Игнорируем слишком короткие слова
            freq[word]++;
            totalWords++;
        }
    }

    // Нормализуем частоты
    for (auto& [w, f] : freq) {
        f /= totalWords;
    }

    return freq;
}

float ContextManager::cosineSimilarity(const std::map<std::string, float>& vec1,
    const std::map<std::string, float>& vec2) {
    if (vec1.empty() || vec2.empty()) {
        return 0.0f;
    }

    float dotProduct = 0.0f;
    float norm1 = 0.0f;
    float norm2 = 0.0f;

    // Вычисляем dot product только для общих слов
    for (const auto& [word, freq1] : vec1) {
        auto it = vec2.find(word);
        if (it != vec2.end()) {
            dotProduct += freq1 * it->second;
        }
        norm1 += freq1 * freq1;
    }

    for (const auto& [word, freq2] : vec2) {
        norm2 += freq2 * freq2;
    }

    if (norm1 == 0.0f || norm2 == 0.0f) {
        return 0.0f;
    }

    return dotProduct / (std::sqrt(norm1) * std::sqrt(norm2));
}

size_t ContextManager::estimateTokenCount(const std::string& text) {
    // Простая оценка: примерно 1 токен на 4 символа для английского/русского текста
    return text.length() / 4;
}

// Альтернативный вариант с более безопасной проверкой:
std::string ContextManager::normalizeText(const std::string& text) {
    std::string normalized;
    normalized.reserve(text.length());

    for (char ch : text) {
        unsigned char c = static_cast<unsigned char>(ch);  // Безопасное приведение

        if (std::isalnum(c) || std::isspace(c)) {
            normalized += static_cast<char>(std::tolower(c));
        }
        else if (c < 128) {  // Только ASCII символы
            normalized += ' '; // Заменяем знаки препинания пробелами
        }
        // Пропускаем не-ASCII символы (UTF-8 части)
    }

    return normalized;
}

std::vector<std::string> ContextManager::extractKeywords(const std::string& query) {
    // Стоп-слова для русского и английского языков
    static const std::unordered_set<std::string> stopWords = {
        "в", "на", "и", "с", "по", "для", "от", "до", "из", "к", "о", "что", "как", "где", "когда",
        "the", "a", "an", "and", "or", "but", "in", "on", "at", "to", "for", "of", "with", "by",
        "is", "are", "was", "were", "be", "been", "have", "has", "had", "do", "does", "did",
        "this", "that", "these", "those", "what", "where", "when", "how", "why"
    };

    std::vector<std::string> keywords;
    std::istringstream stream(query);
    std::string word;

    while (stream >> word) {
        // Очищаем слово от знаков препинания
        std::string cleanWord;
        for (char ch : word) {
            unsigned char c = static_cast<unsigned char>(ch);
            if (std::isalnum(c)) {
                cleanWord += static_cast<char>(std::tolower(c));
            }
        }

        if (cleanWord.length() > 2 && stopWords.find(cleanWord) == stopWords.end()) {
            keywords.push_back(cleanWord);
        }
    }

    return keywords;
}
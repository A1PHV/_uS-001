// ContextManager.h
#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <algorithm>
#include <numeric>

// Структура для хранения документа
struct Document {
    std::string name;                    // Имя файла
    std::string content;                 // Полное содержимое
    std::vector<std::string> chunks;     // Разбитый на части текст
    size_t originalSize;                 // Размер оригинального файла
    std::time_t addedTime;               // Время добавления
};

// Структура для ранжированного чанка
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
    // Конструктор с настраиваемыми параметрами
    explicit ContextManager(size_t maxContextTokens = 3000, size_t maxChunkSize = 800);

    // Добавление документа в контекст
    void addDocument(const std::string& docName, const std::string& content);

    // Получение контекста для запроса
    std::string getContextForQuery(const std::string& query);

    // Получение списка имен документов
    std::vector<std::string> getDocumentNames() const;

    // Получение статистики документов
    std::string getDocumentStats() const;

    // Очистка всех документов
    void clearDocuments();

    // Удаление конкретного документа
    bool removeDocument(const std::string& docName);

    // Настройка параметров
    void setMaxContextTokens(size_t tokens);
    void setMaxChunkSize(size_t size);

private:
    // Параметры
    size_t maxContextTokens;
    size_t maxChunkSize;

    // Хранилище документов
    std::map<std::string, std::shared_ptr<Document>> documents;

    // Мьютекс для потокобезопасности
    mutable std::mutex mtx;

    // Разбиение содержимого на чанки
    std::vector<std::string> chunkContent(const std::string& content);

    // Ранжирование чанков по релевантности
    std::vector<RankedChunk> rankChunksByRelevance(const std::string& query);

    // Оценка релевантности чанка к запросу
    float calculateRelevance(const std::string& query, const std::string& chunk);

    // Создание простого эмбеддинга текста (TF-IDF подобный)
    std::map<std::string, float> createWordFrequencyMap(const std::string& text);

    // Вычисление косинусного сходства
    float cosineSimilarity(const std::map<std::string, float>& vec1,
        const std::map<std::string, float>& vec2);

    // Оценка количества токенов (примерно)
    size_t estimateTokenCount(const std::string& text);

    // Нормализация текста для анализа
    std::string normalizeText(const std::string& text);

    // Извлечение ключевых слов из запроса
    std::vector<std::string> extractKeywords(const std::string& query);
};

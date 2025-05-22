// LLMInterface.h
#pragma once

#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <atomic>

// Включаем API llama.cpp
#include <llama.h>

class LLMInterface {
public:
    // Конструктор принимает путь к модели
    LLMInterface(const std::string& modelPath);
    ~LLMInterface();

    // Установка контекста для запросов
    void setContext(const std::string& context);

    // Генерация ответа на запрос
    std::string generateResponse(
        const std::string& prompt,
        bool streamOutput = false,
        std::function<void(const std::string&)> streamCallback = nullptr
    );

    // Остановка генерации
    void stopGeneration();

    // Сброс контекста
    void resetContext();

    // Информация о модели
    std::string getModelInfo() const;

    // Проверка, загружена ли модель
    bool isLoaded() const;

private:
    // Компоненты llama.cpp
    llama_model* model;
    llama_context* ctx;
    llama_sampler* sampler;

    // Параметры контекста (для старого API)
    llama_context_params ctxParams;

    // Контекстные данные
    std::string contextData;

    // Мьютекс для синхронизации доступа
    mutable std::mutex mtx;

    // Флаг для остановки генерации
    std::atomic<bool> stopRequested;

    // Флаг успешной загрузки
    std::atomic<bool> loaded;

    // Инициализация модели
    void initializeModel(const std::string& modelPath);

    // Инициализация самплера
    void initializeSampler();

    // Токенизация текста
    std::vector<llama_token> tokenize(const std::string& text, bool addBos = false);

    // Детокенизация токенов
    std::string detokenize(const std::vector<llama_token>& tokens);

    // Простое семплирование токенов
    llama_token sampleToken(const float* logits, int n_vocab);

    // Очистка ресурсов
    void cleanup();
};
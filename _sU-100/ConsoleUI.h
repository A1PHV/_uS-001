// ConsoleUI.h
#pragma once

#include <string>
#include <memory>
#include <functional>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

// Предварительные объявления
class LLMInterface;
class ContextManager;

class ConsoleUI {
public:
    ConsoleUI();
    ~ConsoleUI();

    // Запуск интерактивного режима
    void startInteractiveMode(
        std::shared_ptr<LLMInterface> llm,
        std::shared_ptr<ContextManager> contextManager
    );

private:
    // Компоненты системы
    std::shared_ptr<LLMInterface> llm;
    std::shared_ptr<ContextManager> contextManager;

    // Флаги состояния
    std::atomic<bool> running;
    std::atomic<bool> generating;
    std::atomic<bool> stopRequested;

    // Синхронизация
    std::mutex outputMutex;
    std::thread inputMonitorThread;

    // Получение ввода пользователя
    std::string getUserInput();

    // Обработка команд
    bool processCommand(const std::string& command);

    // Отображение справки
    void displayHelp();

    // Обработка запроса к LLM
    void processQuery(const std::string& query);

    // Отображение приветствия
    void displayWelcome();

    // Отображение статистики системы
    void displaySystemStats();

    // Отображение информации о документах
    void displayDocumentInfo();

    // Настройка параметров
    void configureSettings();

    // Поток для мониторинга ввода во время генерации
    void inputMonitorThread_func();

    // Callback для потоковой генерации
    void streamCallback(const std::string& chunk);

    // Форматированный вывод времени
    std::string getCurrentTimeString();

    // Форматирование текста для вывода
    std::string formatOutput(const std::string& text, const std::string& prefix = "");

    // Отображение прогресса
    void showProgress(const std::string& message);

    // Цвета и форматирование (ANSI коды для поддерживающих терминалов)
    static const std::string COLOR_RESET;
    static const std::string COLOR_BOLD;
    static const std::string COLOR_GREEN;
    static const std::string COLOR_BLUE;
    static const std::string COLOR_YELLOW;
    static const std::string COLOR_RED;
    static const std::string COLOR_CYAN;
};
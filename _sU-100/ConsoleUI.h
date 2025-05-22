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

// ��������������� ����������
class LLMInterface;
class ContextManager;

class ConsoleUI {
public:
    ConsoleUI();
    ~ConsoleUI();

    // ������ �������������� ������
    void startInteractiveMode(
        std::shared_ptr<LLMInterface> llm,
        std::shared_ptr<ContextManager> contextManager
    );

private:
    // ���������� �������
    std::shared_ptr<LLMInterface> llm;
    std::shared_ptr<ContextManager> contextManager;

    // ����� ���������
    std::atomic<bool> running;
    std::atomic<bool> generating;
    std::atomic<bool> stopRequested;

    // �������������
    std::mutex outputMutex;
    std::thread inputMonitorThread;

    // ��������� ����� ������������
    std::string getUserInput();

    // ��������� ������
    bool processCommand(const std::string& command);

    // ����������� �������
    void displayHelp();

    // ��������� ������� � LLM
    void processQuery(const std::string& query);

    // ����������� �����������
    void displayWelcome();

    // ����������� ���������� �������
    void displaySystemStats();

    // ����������� ���������� � ����������
    void displayDocumentInfo();

    // ��������� ����������
    void configureSettings();

    // ����� ��� ����������� ����� �� ����� ���������
    void inputMonitorThread_func();

    // Callback ��� ��������� ���������
    void streamCallback(const std::string& chunk);

    // ��������������� ����� �������
    std::string getCurrentTimeString();

    // �������������� ������ ��� ������
    std::string formatOutput(const std::string& text, const std::string& prefix = "");

    // ����������� ���������
    void showProgress(const std::string& message);

    // ����� � �������������� (ANSI ���� ��� �������������� ����������)
    static const std::string COLOR_RESET;
    static const std::string COLOR_BOLD;
    static const std::string COLOR_GREEN;
    static const std::string COLOR_BLUE;
    static const std::string COLOR_YELLOW;
    static const std::string COLOR_RED;
    static const std::string COLOR_CYAN;
};
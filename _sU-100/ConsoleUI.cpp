// ConsoleUI.cpp
#include "ConsoleUI.h"
#include "LLMInterface.h"
#include "ContextManager.h"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <conio.h> // Для Windows, для проверки нажатий клавиш
#pragma warning(disable:4996)


// ANSI цвета (работают в современных терминалах Windows 10+)
const std::string ConsoleUI::COLOR_RESET = "\033[0m";
const std::string ConsoleUI::COLOR_BOLD = "\033[1m";
const std::string ConsoleUI::COLOR_GREEN = "\033[32m";
const std::string ConsoleUI::COLOR_BLUE = "\033[34m";
const std::string ConsoleUI::COLOR_YELLOW = "\033[33m";
const std::string ConsoleUI::COLOR_RED = "\033[31m";
const std::string ConsoleUI::COLOR_CYAN = "\033[36m";

ConsoleUI::ConsoleUI()
    : running(false), generating(false), stopRequested(false) {
}

ConsoleUI::~ConsoleUI() {
    if (running) {
        running = false;
    }

    if (inputMonitorThread.joinable()) {
        inputMonitorThread.join();
    }
}

void ConsoleUI::startInteractiveMode(
    std::shared_ptr<LLMInterface> llm,
    std::shared_ptr<ContextManager> contextManager
) {
    this->llm = llm;
    this->contextManager = contextManager;

    running = true;
    displayWelcome();

    while (running) {
        std::cout << "\n" << COLOR_CYAN << "> " << COLOR_RESET;

        std::string input = getUserInput();

        if (input.empty()) {
            continue;
        }

        // Проверяем, является ли ввод командой
        if (input[0] == '/') {
            if (!processCommand(input)) {
                break; // Команда выхода
            }
        }
        else {
            // Обработка запроса к LLM
            processQuery(input);
        }
    }

    std::cout << "\n" << COLOR_GREEN << "Goodbye! Thank you for using LLM Pipeline!"
        << COLOR_RESET << std::endl;
}

std::string ConsoleUI::getUserInput() {
    std::string input;
    std::getline(std::cin, input);
    return input;
}

bool ConsoleUI::processCommand(const std::string& command) {
    // Разбор команды
    std::istringstream iss(command.substr(1)); // Убираем '/'
    std::string action;
    iss >> action;

    if (action == "exit" || action == "quit") {
        running = false;
        return false;
    }
    else if (action == "help" || action == "h") {
        displayHelp();
    }
    else if (action == "list" || action == "l") {
        displayDocumentInfo();
    }
    else if (action == "clear" || action == "c") {
        contextManager->clearDocuments();
        llm->resetContext();
        std::cout << COLOR_GREEN << "✓ All context cleared" << COLOR_RESET << std::endl;
    }
    else if (action == "remove" || action == "rm") {
        std::string docName;
        std::getline(iss, docName);
        docName.erase(0, docName.find_first_not_of(" \t"));

        if (docName.empty()) {
            std::cout << COLOR_RED << "✗ Error: Please specify document name to remove"
                << COLOR_RESET << std::endl;
            std::cout << "Usage: /remove <document_name>" << std::endl;
        }
        else {
            contextManager->removeDocument(docName);
        }
    }
    else if (action == "reset" || action == "r") {
        llm->resetContext();
        std::cout << COLOR_GREEN << "✓ Model context reset" << COLOR_RESET << std::endl;
    }
    else if (action == "info" || action == "i") {
        displaySystemStats();
    }
    else if (action == "stats" || action == "s") {
        std::cout << contextManager->getDocumentStats() << std::endl;
    }
    else if (action == "config" || action == "set") {
        configureSettings();
    }
    else if (action == "stop") {
        if (generating) {
            std::cout << "\n" << COLOR_YELLOW << "[Generation stopped by user]"
                << COLOR_RESET << std::endl;
            llm->stopGeneration();
            stopRequested = true;
        }
        else {
            std::cout << COLOR_YELLOW << "No active generation to stop" << COLOR_RESET << std::endl;
        }
    }
    else {
        std::cout << COLOR_RED << "✗ Unknown command: " << action << COLOR_RESET << std::endl;
        std::cout << "Type " << COLOR_CYAN << "/help" << COLOR_RESET << " for available commands" << std::endl;
    }

    return true;
}

void ConsoleUI::displayHelp() {
    std::cout << "\n" << COLOR_BOLD << "=== LLM Pipeline Commands ===" << COLOR_RESET << "\n\n";

    std::cout << COLOR_CYAN << "Basic Commands:" << COLOR_RESET << "\n";
    std::cout << "  /help, /h        - Display this help message\n";
    std::cout << "  /exit, /quit     - Exit the program\n\n";

    std::cout << COLOR_CYAN << "Document Management:" << COLOR_RESET << "\n";
    std::cout << "  /list, /l        - List all documents in context\n";
    std::cout << "  /stats, /s       - Show detailed document statistics\n";
    std::cout << "  /remove, /rm <name> - Remove specific document from context\n";
    std::cout << "  /clear, /c       - Remove all documents and clear context\n\n";

    std::cout << COLOR_CYAN << "Model Control:" << COLOR_RESET << "\n";
    std::cout << "  /info, /i        - Show model and system information\n";
    std::cout << "  /reset, /r       - Reset model context (keep documents)\n";
    std::cout << "  /stop            - Stop current text generation\n";
    std::cout << "  /config, /set    - Configure system settings\n\n";

    std::cout << COLOR_YELLOW << "Usage Tips:" << COLOR_RESET << "\n";
    std::cout << "• To ask a question, simply type it and press Enter\n";
    std::cout << "• The model will use loaded documents as context automatically\n";
    std::cout << "• During generation, press Ctrl+C or type '/stop' to interrupt\n";
    std::cout << "• Use specific questions to get better, more focused answers\n";
}

void ConsoleUI::processQuery(const std::string& query) {
    if (!llm->isLoaded()) {
        std::cout << COLOR_RED << "✗ Error: Model is not loaded" << COLOR_RESET << std::endl;
        return;
    }

    generating = true;
    stopRequested = false;

    // Запускаем мониторинг ввода
    inputMonitorThread = std::thread(&ConsoleUI::inputMonitorThread_func, this);

    // Показываем прогресс
    showProgress("Preparing context");

    // Получаем контекст для запроса
    std::string context = contextManager->getContextForQuery(query);
    llm->setContext(context);

    // Отображаем время начала генерации
    std::cout << "\n" << COLOR_BLUE << "[" << getCurrentTimeString() << "] "
        << COLOR_BOLD << "Assistant:" << COLOR_RESET << "\n";

    auto startTime = std::chrono::steady_clock::now();

    // Генерируем ответ с потоковым выводом
    std::string response = llm->generateResponse(
        query,
        true,  // streamOutput
        std::bind(&ConsoleUI::streamCallback, this, std::placeholders::_1)
    );

    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    // Завершаем мониторинг ввода
    generating = false;
    if (inputMonitorThread.joinable()) {
        inputMonitorThread.join();
    }

    // Показываем статистику генерации
    std::cout << "\n\n" << COLOR_YELLOW << "📊 Generation completed in "
        << duration.count() << "ms" << COLOR_RESET << std::endl;
}

void ConsoleUI::displayWelcome() {
    std::cout << "\n" << COLOR_BOLD << "╔══════════════════════════════════════════════════════════╗" << COLOR_RESET << "\n";
    std::cout << COLOR_BOLD << "║" << COLOR_CYAN << "          LLM Pipeline with PDF Document Support          " << COLOR_BOLD << "║" << COLOR_RESET << "\n";
    std::cout << COLOR_BOLD << "╚══════════════════════════════════════════════════════════╝" << COLOR_RESET << "\n\n";

    if (llm && llm->isLoaded()) {
        std::cout << COLOR_GREEN << "✓ Model loaded and ready" << COLOR_RESET << "\n";
    }
    else {
        std::cout << COLOR_RED << "✗ Model not loaded" << COLOR_RESET << "\n";
    }

    auto docNames = contextManager->getDocumentNames();
    if (!docNames.empty()) {
        std::cout << COLOR_GREEN << "✓ " << docNames.size() << " document(s) loaded: ";
        for (size_t i = 0; i < docNames.size(); ++i) {
            std::cout << COLOR_CYAN << docNames[i] << COLOR_RESET;
            if (i < docNames.size() - 1) std::cout << ", ";
        }
        std::cout << "\n";
    }
    else {
        std::cout << COLOR_YELLOW << "⚠ No documents loaded" << COLOR_RESET << "\n";
    }

    std::cout << "\nType your question or " << COLOR_CYAN << "/help" << COLOR_RESET
        << " for commands\n";
}

void ConsoleUI::displaySystemStats() {
    std::cout << "\n" << COLOR_BOLD << "=== SYSTEM INFORMATION ===" << COLOR_RESET << "\n\n";

    // Информация о модели
    if (llm && llm->isLoaded()) {
        std::cout << COLOR_GREEN << "📱 Model Status: Loaded" << COLOR_RESET << "\n";
        std::cout << llm->getModelInfo() << "\n\n";
    }
    else {
        std::cout << COLOR_RED << "📱 Model Status: Not Loaded" << COLOR_RESET << "\n\n";
    }

    // Статистика документов
    auto docNames = contextManager->getDocumentNames();
    std::cout << COLOR_BLUE << "📚 Documents: " << docNames.size() << " loaded" << COLOR_RESET << "\n";

    if (!docNames.empty()) {
        for (const auto& name : docNames) {
            std::cout << "   • " << COLOR_CYAN << name << COLOR_RESET << "\n";
        }
    }

    std::cout << "\n" << COLOR_BLUE << "⏰ Current Time: " << getCurrentTimeString() << COLOR_RESET << "\n";
}

void ConsoleUI::displayDocumentInfo() {
    auto docNames = contextManager->getDocumentNames();

    if (docNames.empty()) {
        std::cout << COLOR_YELLOW << "No documents currently loaded" << COLOR_RESET << std::endl;
        std::cout << "Place PDF files in the 'documents' folder and restart the application" << std::endl;
        return;
    }

    std::cout << "\n" << COLOR_BOLD << "=== LOADED DOCUMENTS ===" << COLOR_RESET << "\n";
    std::cout << COLOR_GREEN << "Total: " << docNames.size() << " document(s)" << COLOR_RESET << "\n\n";

    for (size_t i = 0; i < docNames.size(); ++i) {
        std::cout << COLOR_CYAN << (i + 1) << ". " << docNames[i] << COLOR_RESET << "\n";
    }

    std::cout << "\nUse " << COLOR_CYAN << "/stats" << COLOR_RESET
        << " for detailed document statistics\n";
    std::cout << "Use " << COLOR_CYAN << "/remove <name>" << COLOR_RESET
        << " to remove a specific document\n";
}

void ConsoleUI::configureSettings() {
    std::cout << "\n" << COLOR_BOLD << "=== CONFIGURATION ===" << COLOR_RESET << "\n\n";

    std::cout << "Available settings:\n";
    std::cout << "1. Max context tokens (current: affects how much document content to include)\n";
    std::cout << "2. Max chunk size (current: how documents are split)\n";
    std::cout << "3. Back to main menu\n\n";

    std::cout << "Select option (1-3): ";
    std::string choice;
    std::getline(std::cin, choice);

    if (choice == "1") {
        std::cout << "Enter max context tokens (1000-8000): ";
        std::string input;
        std::getline(std::cin, input);

        try {
            size_t tokens = std::stoul(input);
            if (tokens >= 1000 && tokens <= 8000) {
                contextManager->setMaxContextTokens(tokens);
                std::cout << COLOR_GREEN << "✓ Max context tokens set to " << tokens << COLOR_RESET << std::endl;
            }
            else {
                std::cout << COLOR_RED << "✗ Invalid range. Use 1000-8000" << COLOR_RESET << std::endl;
            }
        }
        catch (...) {
            std::cout << COLOR_RED << "✗ Invalid number format" << COLOR_RESET << std::endl;
        }
    }
    else if (choice == "2") {
        std::cout << "Enter max chunk size in characters (500-2000): ";
        std::string input;
        std::getline(std::cin, input);

        try {
            size_t size = std::stoul(input);
            if (size >= 500 && size <= 2000) {
                contextManager->setMaxChunkSize(size);
                std::cout << COLOR_GREEN << "✓ Max chunk size set to " << size << COLOR_RESET << std::endl;
            }
            else {
                std::cout << COLOR_RED << "✗ Invalid range. Use 500-2000" << COLOR_RESET << std::endl;
            }
        }
        catch (...) {
            std::cout << COLOR_RED << "✗ Invalid number format" << COLOR_RESET << std::endl;
        }
    }
}

void ConsoleUI::inputMonitorThread_func() {
    while (generating && !stopRequested) {
        // Проверяем, нажата ли клавиша (Windows specific)
        if (_kbhit()) {
            char ch = _getch();
            if (ch == 27) { // ESC key
                std::lock_guard<std::mutex> lock(outputMutex);
                std::cout << "\n" << COLOR_YELLOW << "[Generation stopped by ESC key]" << COLOR_RESET << "\n";
                llm->stopGeneration();
                stopRequested = true;
                break;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void ConsoleUI::streamCallback(const std::string& chunk) {
    std::lock_guard<std::mutex> lock(outputMutex);
    std::cout << chunk;
    std::cout.flush();
}

std::string ConsoleUI::getCurrentTimeString() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%H:%M:%S");
    return ss.str();
}

std::string ConsoleUI::formatOutput(const std::string& text, const std::string& prefix) {
    std::stringstream ss;
    std::istringstream stream(text);
    std::string line;

    while (std::getline(stream, line)) {
        ss << prefix << line << "\n";
    }

    return ss.str();
}

void ConsoleUI::showProgress(const std::string& message) {
    std::cout << COLOR_YELLOW << "⏳ " << message << "..." << COLOR_RESET << std::endl;
}
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <memory>
#include <thread>
#include <chrono>

#include "LLMInterface.h"
#include "PDFProcessor.h"
#include "ContextManager.h"
#include "ConsoleUI.h"
#include <consoleapi2.h>
#include <WinNls.h>



namespace fs = std::filesystem;

// Функция для поиска модели
std::string findModelFile() {
    const std::vector<std::string> searchPaths = {
        "models/",
        "./",
        "../models/"
    };

    const std::vector<std::string> modelExtensions = {
        ".gguf",
        ".bin"
    };

    for (const auto& path : searchPaths) {
        if (!fs::exists(path) || !fs::is_directory(path)) {
            continue;
        }

        for (const auto& entry : fs::directory_iterator(path)) {
            if (entry.is_regular_file()) {
                std::string ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

                for (const auto& validExt : modelExtensions) {
                    if (ext == validExt) {
                        return entry.path().string();
                    }
                }
            }
        }
    }

    return "";
}

// Функция для создания необходимых директорий
void createDirectories() {
    const std::vector<std::string> directories = {
        "models",
        "documents",
        "tessdata"
    };

    for (const auto& dir : directories) {
        if (!fs::exists(dir)) {
            fs::create_directory(dir);
            std::cout << "Created directory: " << dir << std::endl;
        }
    }
}

// Функция для обработки PDF документов
void processDocuments(std::shared_ptr<PDFProcessor> pdfProcessor,
    std::shared_ptr<ContextManager> contextManager) {
    const std::string documentsDir = "documents";

    if (!fs::exists(documentsDir) || !fs::is_directory(documentsDir)) {
        std::cout << "Warning: Documents directory not found: " << documentsDir << std::endl;
        return;
    }

    std::vector<fs::path> pdfFiles;
    for (const auto& entry : fs::directory_iterator(documentsDir)) {
        if (entry.path().extension() == ".pdf") {
            pdfFiles.push_back(entry.path());
        }
    }

    if (pdfFiles.empty()) {
        std::cout << "No PDF files found in documents directory" << std::endl;
        return;
    }

    std::cout << "\n=== Processing PDF Documents ===" << std::endl;
    std::cout << "Found " << pdfFiles.size() << " PDF file(s) to process" << std::endl;

    int processed = 0;
    int failed = 0;

    for (const auto& pdfPath : pdfFiles) {
        std::cout << "\n(" << (processed + failed + 1) << "/" << pdfFiles.size()
            << ") Processing: " << pdfPath.filename() << std::endl;

        try {
            auto startTime = std::chrono::steady_clock::now();

            // Извлекаем текст из PDF
            std::string pdfText = pdfProcessor->extractText(pdfPath.string());

            auto endTime = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

            if (!pdfText.empty() && pdfText.find("Error:") != 0) {
                // Добавляем в контекст менеджер
                contextManager->addDocument(pdfPath.filename().string(), pdfText);
                processed++;

                std::cout << "✓ Successfully processed in " << duration.count()
                    << "ms (" << pdfText.length() << " characters)" << std::endl;
            }
            else {
                std::cout << "✗ Failed to extract text: " << pdfText.substr(0, 100) << std::endl;
                failed++;
            }
        }
        catch (const std::exception& e) {
            std::cout << "✗ Error processing file: " << e.what() << std::endl;
            failed++;
        }
    }

    std::cout << "\n=== Processing Summary ===" << std::endl;
    std::cout << "Successfully processed: " << processed << " files" << std::endl;
    std::cout << "Failed: " << failed << " files" << std::endl;

    if (processed > 0) {
        std::cout << "✓ Documents are ready for use as context" << std::endl;
    }
}

// Функция отображения системной информации
void displaySystemInfo() {
    std::cout << "\n=== LLM Pipeline System Information ===" << std::endl;
    std::cout << "Version: 1.0.0" << std::endl;
    std::cout << "Build Date: " << __DATE__ << " " << __TIME__ << std::endl;
    std::cout << "Hardware threads: " << std::thread::hardware_concurrency() << std::endl;

    // Проверка доступности директорий
    std::cout << "\nDirectory Status:" << std::endl;
    std::cout << "Models: " << (fs::exists("models") ? "✓" : "✗") << std::endl;
    std::cout << "Documents: " << (fs::exists("documents") ? "✓" : "✗") << std::endl;
    std::cout << "Tessdata: " << (fs::exists("tessdata") ? "✓" : "✗") << std::endl;
}


int main(int argc, char* argv[]) {
#ifdef _WIN32
    // Исправление кодировки консоли
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    setlocale(LC_ALL, "ru_RU.UTF-8");
#endif
    try {
        std::cout << "🚀 LLM Pipeline Starting..." << std::endl;

        // Отображаем системную информацию
        displaySystemInfo();

        // Создаем необходимые директории
        createDirectories();

        // Ищем модель
        std::cout << "\n=== Model Loading ===" << std::endl;
        std::string modelPath = findModelFile();

        if (modelPath.empty()) {
            std::cout << "❌ No model found in the following locations:" << std::endl;
            std::cout << "   • models/*.gguf" << std::endl;
            std::cout << "   • models/*.bin" << std::endl;
            std::cout << "\nPlease download a compatible model and place it in the models/ directory." << std::endl;
            std::cout << "\nRecommended models:" << std::endl;
            std::cout << "   • TinyLlama-1.1B-Chat-v1.0.Q4_K_M.gguf (~1GB)" << std::endl;
            std::cout << "   • Mistral-7B-Instruct-v0.2.Q4_K_M.gguf (~4GB)" << std::endl;

            std::cout << "\nPress Enter to exit...";
            std::cin.get();
            return 1;
        }

        std::cout << "Found model: " << modelPath << std::endl;

        // Инициализация компонентов
        std::cout << "\n=== Component Initialization ===" << std::endl;

        // LLM Interface
        std::cout << "Initializing LLM Interface..." << std::endl;
        auto llm = std::make_shared<LLMInterface>(modelPath);

        if (!llm->isLoaded()) {
            std::cerr << "❌ Failed to load LLM model" << std::endl;
            return 1;
        }

        // PDF Processor
        std::cout << "Initializing PDF Processor..." << std::endl;
        auto pdfProcessor = std::make_shared<PDFProcessor>();

        // Context Manager
        std::cout << "Initializing Context Manager..." << std::endl;
        auto contextManager = std::make_shared<ContextManager>(
            3000, // max context tokens
            800   // max chunk size
            );

        // Console UI
        std::cout << "Initializing Console UI..." << std::endl;
        auto consoleUI = std::make_shared<ConsoleUI>();

        std::cout << "✓ All components initialized successfully" << std::endl;

        // Обработка PDF документов
        processDocuments(pdfProcessor, contextManager);

        // Небольшая пауза перед запуском UI
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // Запуск интерактивного режима
        std::cout << "\n=== Starting Interactive Mode ===" << std::endl;
        consoleUI->startInteractiveMode(llm, contextManager);

    }
    catch (const std::exception& e) {
        std::cerr << "\n❌ Fatal Error: " << e.what() << std::endl;
        std::cout << "\nPress Enter to exit...";
        std::cin.get();
        return 1;
    }

    std::cout << "\n✓ LLM Pipeline terminated successfully" << std::endl;
    return 0;
}
// PDFProcessor.h
#pragma once

#include <string>
#include <vector>
#include <memory>

// Предварительное объявление для Tesseract
namespace tesseract {
    class TessBaseAPI;
}

// Используем предварительные объявления для Poppler
namespace poppler {
    class document;
    class page;
    class ustring;
}

// Предварительное объявление для Leptonica
struct Pix;

class PDFProcessor {
public:
    PDFProcessor();
    ~PDFProcessor();

    // Извлечение текста из PDF документа
    std::string extractText(const std::string& pdfPath);

private:
    // Проверка существования файла
    bool fileExists(const std::string& filePath);

    // Инициализация OCR движка
    void initOCR();

    // Извлечение текста из страницы PDF
    std::string extractTextFromPage(poppler::page* page);

    // Проверка, является ли страница сканом
    bool isScannedPage(poppler::page* page);

    // Извлечение текста из скана с помощью OCR
    std::string extractTextWithOCR(poppler::page* page);

    // Рендеринг страницы PDF в изображение для OCR
    Pix* renderPageToImage(poppler::page* page, int dpi = 300);

    // Преобразование poppler::ustring в std::string
    std::string ustringToString(const poppler::ustring& ustr);

    // OCR движок
    std::unique_ptr<tesseract::TessBaseAPI> tessApi;
};
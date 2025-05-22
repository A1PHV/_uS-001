// PDFProcessor.cpp
#include "PDFProcessor.h"
#include <iostream>
#include <fstream>
#include <filesystem>

// �������� ��������� Poppler
#include <poppler/cpp/poppler-document.h>
#include <poppler/cpp/poppler-page.h>
#include <poppler/cpp/poppler-page-renderer.h>
#include <poppler/cpp/poppler-global.h>

// �������� ��������� Tesseract � Leptonica
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>

namespace fs = std::filesystem;

PDFProcessor::PDFProcessor() {
    initOCR();
}

PDFProcessor::~PDFProcessor() {
    if (tessApi) {
        tessApi->End();
    }
}

bool PDFProcessor::fileExists(const std::string& filePath) {
    return fs::exists(filePath);
}

void PDFProcessor::initOCR() {
    try {
        tessApi = std::make_unique<tesseract::TessBaseAPI>();

        // ������������� OCR � ���������� �������� � ����������� ������
        if (tessApi->Init(nullptr, "rus+eng")) {
            std::cerr << "Could not initialize Tesseract OCR engine" << std::endl;
            tessApi.reset();
        }

        // ��������� ������ ����������� �������� ��� ����������
        if (tessApi) {
            tessApi->SetPageSegMode(tesseract::PSM_AUTO);
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error initializing OCR: " << e.what() << std::endl;
        tessApi.reset();
    }
}

std::string PDFProcessor::extractText(const std::string& pdfPath) {
    if (!fileExists(pdfPath)) {
        return "Error: File not found: " + pdfPath;
    }

    try {
        // ��������� ��������
        std::unique_ptr<poppler::document> doc(poppler::document::load_from_file(pdfPath));

        if (!doc) {
            return "Error: Failed to open PDF file: " + pdfPath;
        }

        int pageCount = doc->pages();
        std::cout << "PDF loaded successfully. Total pages: " << pageCount << std::endl;

        std::stringstream result;
        result << "Extracted text from: " << pdfPath << "\n\n";

        // ������������ ������ ��������
        for (int i = 0; i < pageCount; ++i) {
            std::cout << "Processing page " << (i + 1) << " of " << pageCount << "\r";
            std::cout.flush();

            // ������� ��������
            std::unique_ptr<poppler::page> page(doc->create_page(i));

            if (!page) {
                std::cerr << "\nError: Failed to load page " << (i + 1) << std::endl;
                continue;
            }

            // ��������� ����� ��������
            result << "=== Page " << (i + 1) << " ===\n";

            // ��������� �����
            std::string pageText;
            if (isScannedPage(page.get())) {
                pageText = extractTextWithOCR(page.get());
            }
            else {
                pageText = extractTextFromPage(page.get());
            }

            result << pageText << "\n\n";
        }

        std::cout << "\nText extraction completed." << std::endl;
        return result.str();
    }
    catch (const std::exception& e) {
        std::cerr << "Error processing PDF: " << e.what() << std::endl;
        return "Error processing PDF: " + std::string(e.what());
    }
}

std::string PDFProcessor::extractTextFromPage(poppler::page* page) {
    if (!page) {
        return "";
    }

    try {
        // �������� ���� ����� �������� � ���������� �����������
        poppler::ustring utext = page->text(poppler::rectf(), poppler::page::physical_layout);
        return ustringToString(utext);
    }
    catch (const std::exception& e) {
        std::cerr << "Error extracting text: " << e.what() << std::endl;
        return "";
    }
}

std::string PDFProcessor::ustringToString(const poppler::ustring& ustr) {
    // to_utf8() ���������� std::vector<char>, � �� ������ � ������� c_str()
    std::vector<char> utf8Data = ustr.to_utf8();

    if (utf8Data.empty()) {
        return "";
    }

    // ������� ������ �� ������ �������
    return std::string(utf8Data.data(), utf8Data.size());
}

bool PDFProcessor::isScannedPage(poppler::page* page) {
    if (!page) {
        return false;
    }

    // �������� ����� ��������
    std::string text = extractTextFromPage(page);

    // ������� ���������: ���� ������ ���� ��� ��� ���, �������� ��� ����
    return text.length() < 100;
}

std::string PDFProcessor::extractTextWithOCR(poppler::page* page) {
    if (!tessApi) {
        return "OCR not initialized";
    }

    // �������� ����������� ��������
    Pix* pixImage = renderPageToImage(page);
    if (!pixImage) {
        return "Error: Failed to render page for OCR";
    }

    try {
        // ������������� ����������� ��� OCR
        tessApi->SetImage(pixImage);

        // ��������� OCR
        char* ocrText = tessApi->GetUTF8Text();
        if (!ocrText) {
            pixDestroy(&pixImage);
            return "Error: OCR failed to extract text";
        }

        // �������� ���������
        std::string result(ocrText);

        // ����������� �������
        delete[] ocrText;
        pixDestroy(&pixImage);

        return result;
    }
    catch (const std::exception& e) {
        pixDestroy(&pixImage);
        return "OCR Error: " + std::string(e.what());
    }
}

Pix* PDFProcessor::renderPageToImage(poppler::page* page, int dpi) {
    if (!page) {
        return nullptr;
    }

    try {
        // ������� ��������
        poppler::page_renderer renderer;
        renderer.set_render_hint(poppler::page_renderer::antialiasing);
        renderer.set_render_hint(poppler::page_renderer::text_antialiasing);

        // �������� �������� � �����������
        poppler::image img = renderer.render_page(page, dpi, dpi);

        if (!img.is_valid()) {
            std::cerr << "Failed to render page to image" << std::endl;
            return nullptr;
        }

        // ������� PIX ����������� ��� Tesseract
        Pix* pixImage = pixCreate(img.width(), img.height(), 32);
        if (!pixImage) {
            std::cerr << "Failed to create PIX image" << std::endl;
            return nullptr;
        }

        // ��������� PIX ������� �� �����������
        uint32_t* pixData = pixGetData(pixImage);
        int wpl = pixGetWpl(pixImage);

        for (int y = 0; y < img.height(); ++y) {
            uint32_t* line = pixData + y * wpl;
            for (int x = 0; x < img.width(); ++x) {
                // �������� ������ ������� �� ����������� Poppler
                int idx = y * img.width() * 4 + x * 4;
                uint8_t r = img.data()[idx];
                uint8_t g = img.data()[idx + 1];
                uint8_t b = img.data()[idx + 2];
                uint8_t a = img.data()[idx + 3];

                // ������� 32-������ ������� ��� PIX
                // PIX ���������� ������ RGBA, ��� R ��������� � ������� �����
                line[x] = (r << 24) | (g << 16) | (b << 8) | a;
            }
        }

        return pixImage;
    }
    catch (const std::exception& e) {
        std::cerr << "Error rendering page: " << e.what() << std::endl;
        return nullptr;
    }
}
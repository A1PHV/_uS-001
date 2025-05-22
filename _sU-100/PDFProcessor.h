// PDFProcessor.h
#pragma once

#include <string>
#include <vector>
#include <memory>

// ��������������� ���������� ��� Tesseract
namespace tesseract {
    class TessBaseAPI;
}

// ���������� ��������������� ���������� ��� Poppler
namespace poppler {
    class document;
    class page;
    class ustring;
}

// ��������������� ���������� ��� Leptonica
struct Pix;

class PDFProcessor {
public:
    PDFProcessor();
    ~PDFProcessor();

    // ���������� ������ �� PDF ���������
    std::string extractText(const std::string& pdfPath);

private:
    // �������� ������������� �����
    bool fileExists(const std::string& filePath);

    // ������������� OCR ������
    void initOCR();

    // ���������� ������ �� �������� PDF
    std::string extractTextFromPage(poppler::page* page);

    // ��������, �������� �� �������� ������
    bool isScannedPage(poppler::page* page);

    // ���������� ������ �� ����� � ������� OCR
    std::string extractTextWithOCR(poppler::page* page);

    // ��������� �������� PDF � ����������� ��� OCR
    Pix* renderPageToImage(poppler::page* page, int dpi = 300);

    // �������������� poppler::ustring � std::string
    std::string ustringToString(const poppler::ustring& ustr);

    // OCR ������
    std::unique_ptr<tesseract::TessBaseAPI> tessApi;
};
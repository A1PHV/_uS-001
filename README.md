# _uS-001: LLM Pipeline с обработкой PDF документов

C++ приложение для работы с локальными LLM моделями и автоматического извлечения текста из PDF документов для использования в качестве контекста через консольный интерфейс.

## Возможности

- Запуск локальных LLM моделей через llama.cpp (поддержка GGUF формата)
- Извлечение текста из PDF документов (включая сканы через OCR)
- Интеллектуальное ранжирование контекста по релевантности запроса
- Интерактивный консольный интерфейс с командами управления
- Потоковая генерация ответов в реальном времени
- Полностью локальная работа без интернета

## Требования

### Системные требования
- Windows 10/11 (проект настроен для Visual Studio)
- Visual Studio 2019/2022 с поддержкой C++17
- Минимум 8 ГБ RAM (16+ ГБ рекомендуется для больших моделей)
- ~10 ГБ свободного места на диске для моделей

### Зависимости (устанавливаются автоматически через vcpkg)
- llama-cpp - для работы с LLM моделями
- poppler - для обработки PDF файлов
- tesseract - для OCR сканированных документов
- leptonica - зависимость для Tesseract

## Установка

### 1. Установка и настройка vcpkg

```bash
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install
```

### 2. Установка всех зависимостей через vcpkg

```bash
# Установка основных компонентов
.\vcpkg install llama-cpp:x64-windows
.\vcpkg install poppler:x64-windows  
.\vcpkg install tesseract:x64-windows
.\vcpkg install leptonica:x64-windows

# Проверка установки
.\vcpkg list | findstr -i "llama poppler tesseract leptonica"
```

### 3. Клонирование проекта

```bash
git clone https://github.com/yourusername/_uS-001.git
cd _uS-001
```

### 4. Настройка Visual Studio

1. Откройте `_sU-100.sln` в Visual Studio
2. Убедитесь, что выбрана конфигурация **Release** и платформа **x64**
3. Проект должен автоматически подхватить зависимости через vcpkg integration

**Если автоматическая интеграция не работает:**

Правый клик на проекте → Properties:

**C/C++ → General → Additional Include Directories:**
```
$(VCPKG_ROOT)\installed\x64-windows\include
```

**Linker → General → Additional Library Directories:**
```
$(VCPKG_ROOT)\installed\x64-windows\lib
```

**Linker → Input → Additional Dependencies:**
```
llama.lib
poppler-cpp.lib
tesseract50.lib
leptonica-1.82.0.lib
```

5. Соберите проект (Build → Build Solution или Ctrl+Shift+B)

### 5. Подготовка языковых данных для OCR

Скачайте языковые файлы для Tesseract и поместите в папку `tessdata/`:

```bash
mkdir tessdata
```

Скачайте с [tessdata](https://github.com/tesseract-ocr/tessdata):
- `eng.traineddata` - для английского языка
- `rus.traineddata` - для русского языка

### 6. Получение LLM модели

Скачайте модель в формате GGUF и поместите в папку `models/`:

```bash
mkdir models
```

**Рекомендуемые модели:**
- [TinyLlama-1.1B-Chat-v1.0](https://huggingface.co/TinyLlama/TinyLlama-1.1B-Chat-v1.0-GGUF) (~1GB) - для тестирования
- [Mistral-7B-Instruct-v0.2](https://huggingface.co/mistralai/Mistral-7B-Instruct-v0.2-GGUF) (~4GB) - рекомендуется  
- [Llama-2-7B-Chat](https://huggingface.co/meta-llama/Llama-2-7b-chat-hf) (~3.5GB) - альтернатива

Загрузите файл `.gguf` и поместите в папку `models/`.

## Использование

### Подготовка документов
1. Поместите PDF документы в папку `documents/`
2. Программа автоматически обработает их при запуске

### Запуск
1. Запустите `_sU-100.exe`
2. Программа автоматически:
   - Найдет и загрузит модель из `models/`
   - Обработает PDF файлы из `documents/`
   - Индексирует контент для быстрого поиска
   - Запустит интерактивный режим

### Доступные команды

```bash
# Основные команды
/help, /h        - Показать справку по командам
/exit, /quit     - Выйти из программы

# Управление документами  
/list, /l        - Список загруженных документов
/stats, /s       - Детальная статистика документов
/remove <name>   - Удалить конкретный документ
/clear, /c       - Очистить все документы

# Управление моделью
/info, /i        - Информация о модели и системе
/reset, /r       - Сбросить контекст модели
/config, /set    - Настройка параметров
/stop            - Остановить генерацию текста
```

### Примеры использования

```bash
# Простые вопросы
> Привет
> Что такое машинное обучение?

# Вопросы по документам
> Что содержится в загруженных документах?
> Найди информацию о технических характеристиках
> Объясни основные выводы из документа
> Какие выводы можно сделать из анализа данных?

# Управление системой
> /list                    # Посмотреть загруженные документы
> /stats                   # Детальная статистика
> /remove filename.pdf     # Удалить конкретный документ
> /config                  # Настроить параметры
```

## Структура проекта

```
_uS-001/
├── src/                        # Исходные файлы
│   ├── _sU-100.cpp            # Основной файл приложения
│   ├── LLMInterface.cpp       # Интерфейс для работы с LLM
│   ├── LLMInterface.h
│   ├── PDFProcessor.cpp       # Обработка PDF документов
│   ├── PDFProcessor.h
│   ├── ContextManager.cpp     # Управление контекстом и поиском
│   ├── ContextManager.h
│   ├── ConsoleUI.cpp          # Консольный интерфейс
│   └── ConsoleUI.h
├── models/                     # LLM модели (.gguf)
├── documents/                  # PDF документы для обработки
├── tessdata/                   # Языковые данные для OCR
├── _sU-100.sln               # Файл проекта Visual Studio
└── README.md
```

## Архитектура системы

### Компоненты

1. **LLMInterface** - работа с локальными моделями через llama.cpp
2. **PDFProcessor** - извлечение текста из PDF (Poppler + Tesseract OCR)
3. **ContextManager** - индексация и поиск релевантного контекста
4. **ConsoleUI** - интерактивный интерфейс с поддержкой команд

### Алгоритм работы

1. **Инициализация**: загрузка модели → обработка PDF → индексация
2. **Запрос пользователя**: поиск релевантных фрагментов → сборка контекста
3. **Генерация**: LLM генерирует ответ на основе контекста
4. **Вывод**: потоковое отображение результата

## Устранение неполадок

### Ошибки установки vcpkg

**"Package not found":**
```bash
# Обновите vcpkg
cd vcpkg
git pull
.\vcpkg update

# Переустановите пакет
.\vcpkg remove llama-cpp:x64-windows
.\vcpkg install llama-cpp:x64-windows
```

### Ошибки компиляции

**"llama.h not found":**
```bash
# Проверьте интеграцию vcpkg
.\vcpkg integrate install

# Убедитесь что llama-cpp установлен
.\vcpkg list | findstr llama
```

**Linker errors:**
- Убедитесь, что платформа установлена как **x64**
- Проверьте что конфигурация **Release**
- Переустановите пакеты vcpkg

### Ошибки выполнения

**"Failed to load model":**
- Проверьте, что файл `.gguf` находится в папке `models/`
- Убедитесь, что достаточно RAM для модели
- Попробуйте модель меньшего размера (например, TinyLlama)

**"PDF processing failed":**
- Убедитесь, что языковые файлы находятся в `tessdata/`
- Проверьте, что PDF файлы не повреждены
- Для сканов убедитесь, что качество изображения достаточное

**Медленная работа:**
- Используйте SSD вместо HDD
- Увеличьте объем RAM
- Попробуйте модель с большей квантизацией (Q4 вместо Q8)

## Конфигурация

### Изменение параметров модели

В `LLMInterface.cpp`:
```cpp
// Размер контекста (больше = больше памяти)
ctxParams.n_ctx = 2048;

// Параметры генерации
llama_sampler_chain_add(sampler, llama_sampler_init_top_k(40));    // Top-K
llama_sampler_chain_add(sampler, llama_sampler_init_top_p(0.9, 1)); // Top-P  
llama_sampler_chain_add(sampler, llama_sampler_init_temp(0.8));    // Temperature
```

### Настройка контекста

В `ContextManager.cpp`:
```cpp
// Максимальный размер контекста в токенах
maxContextTokens = 3000;

// Размер chunk'а документа
maxChunkSize = 800;
```

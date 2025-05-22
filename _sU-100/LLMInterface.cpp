// LLMInterface.cpp - Старое API для версии 4743
#include "LLMInterface.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <random>

LLMInterface::LLMInterface(const std::string& modelPath)
    : model(nullptr), ctx(nullptr), sampler(nullptr), stopRequested(false), loaded(false) {
    try {
        initializeModel(modelPath);
        initializeSampler();
        loaded = true;
        std::cout << "✓ LLM Interface initialized successfully" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "✗ Error initializing LLM: " << e.what() << std::endl;
        cleanup();
        throw;
    }
}

LLMInterface::~LLMInterface() {
    cleanup();
}

void LLMInterface::cleanup() {
    loaded = false;

    if (sampler) {
        llama_sampler_free(sampler);
        sampler = nullptr;
    }

    if (ctx) {
        llama_free(ctx);
        ctx = nullptr;
    }

    if (model) {
        llama_model_free(model);
        model = nullptr;
    }

    llama_backend_free();
}

void LLMInterface::initializeModel(const std::string& modelPath) {
    std::cout << "Initializing llama.cpp backend..." << std::endl;
    llama_backend_init();

    std::cout << "Loading model: " << modelPath << std::endl;

    // Параметры модели
    llama_model_params modelParams = llama_model_default_params();
    modelParams.n_gpu_layers = 0; // CPU-only

    // Загрузка модели
    model = llama_model_load_from_file(modelPath.c_str(), modelParams);

    if (!model) {
        throw std::runtime_error("Failed to load model from: " + modelPath);
    }

    std::cout << "Model loaded. Creating context..." << std::endl;

    // Параметры контекста
    ctxParams = llama_context_default_params();
    ctxParams.n_ctx = 2048;
    ctxParams.n_batch = 128;
    ctxParams.n_threads = std::min(4u, std::thread::hardware_concurrency());

    // Создание контекста
    ctx = llama_init_from_model(model, ctxParams);

    if (!ctx) {
        throw std::runtime_error("Failed to create context from model");
    }

    std::cout << "Context created successfully" << std::endl;
    std::cout << "Context size: " << llama_n_ctx(ctx) << " tokens" << std::endl;
}

void LLMInterface::initializeSampler() {
    std::cout << "Initializing sampler..." << std::endl;

    // В старой версии семплер может отсутствовать - используем простую генерацию
    sampler = nullptr;

    std::cout << "Sampler configured (manual sampling)" << std::endl;
}

void LLMInterface::setContext(const std::string& context) {
    std::lock_guard<std::mutex> lock(mtx);
    contextData = context;

    if (!context.empty()) {
        std::cout << "Context set: " << context.length() << " characters" << std::endl;
    }
}

// Упрощенная реализация generateResponse без проблемного batch API
std::string LLMInterface::generateResponse(
    const std::string& prompt,
    bool streamOutput,
    std::function<void(const std::string&)> streamCallback
) {
    if (!loaded || !model || !ctx) {
        return "Error: Model not properly loaded";
    }

    std::lock_guard<std::mutex> lock(mtx);
    stopRequested = false;

    try {
        // Формируем полный промпт с контекстом
        std::string fullPrompt;
        if (!contextData.empty() && contextData.length() < 1000) {
            // Берем только самую релевантную часть контекста
            std::string shortContext = contextData.substr(0, 500);
            fullPrompt = "Based on this context: " + shortContext +
                "\n\nQuestion: " + prompt +
                "\nAnswer: ";
        }
        else {
            fullPrompt = prompt + "\nAnswer: ";
        }

        std::cout << "Processing prompt (" << fullPrompt.length() << " chars)..." << std::endl;

        // Токенизируем
        std::vector<llama_token> tokens = tokenize(fullPrompt, true);

        if (tokens.empty()) {
            return "Error: Failed to tokenize prompt";
        }

        // Ограничиваем размер промпта
        if (tokens.size() > 512) {
            tokens.resize(512);
            std::cout << "Prompt truncated to 512 tokens" << std::endl;
        }

        std::cout << "Tokenized: " << tokens.size() << " tokens" << std::endl;

        // Очищаем контекст
        llama_kv_cache_clear(ctx);

        // УПРОЩЕННАЯ обработка промпта - по одному токену
        for (size_t i = 0; i < tokens.size(); ++i) {
            // Создаем минимальный batch для одного токена
            llama_batch batch = llama_batch_init(1, 0, 1);

            // Полная инициализация всех полей
            memset(&batch, 0, sizeof(batch));

            batch.n_tokens = 1;
            batch.token = (llama_token*)malloc(sizeof(llama_token));
            batch.pos = (llama_pos*)malloc(sizeof(llama_pos));
            batch.logits = (int8_t*)malloc(sizeof(int8_t));

            batch.token[0] = tokens[i];
            batch.pos[0] = i;
            batch.logits[0] = (i == tokens.size() - 1) ? 1 : 0;

            int result = llama_decode(ctx, batch);

            // Освобождаем память
            free(batch.token);
            free(batch.pos);
            free(batch.logits);

            if (result != 0) {
                std::cerr << "Failed to process token " << i << std::endl;
                return "Error: Failed to process prompt";
            }
        }

        std::cout << "Prompt processed, generating response..." << std::endl;

        // Генерация ответа
        std::string response;
        const int maxTokens = 500; // Увеличиваем до 500 токенов

        for (int i = 0; i < maxTokens && !stopRequested; ++i) {
            // Получаем логиты
            const float* logits = llama_get_logits(ctx);
            if (!logits) {
                std::cout << "Failed to get logits" << std::endl;
                break;
            }

            //// ДОБАВЬТЕ ЭТО - проверка первых нескольких логитов:
            //std::cout << "First 5 logits: ";
            //for (int j = 0; j < 5; ++j) {
            //    std::cout << logits[j] << " ";
            //}
            //std::cout << std::endl;

            // Простейшее семплирование - берем топ токен
           // ИСПРАВЛЕННОЕ семплирование
            const int n_vocab = llama_vocab_n_tokens(llama_model_get_vocab(model));

            llama_token nextToken = 0;
            float maxProb = -1e10f;

            // Ищем токен с максимальной вероятностью среди ВСЕХ токенов
            for (int j = 0; j < n_vocab; ++j) {
                if (std::isfinite(logits[j]) && logits[j] > maxProb) {
                    maxProb = logits[j];
                    nextToken = j;
                }
            }

            // ДОБАВИМ отладку:
            std::cout << "Selected token " << nextToken << " with logit " << maxProb << std::endl;

            // Проверяем что токен валидный
            if (nextToken >= n_vocab || nextToken < 0) {
                std::cout << "Invalid token selected, using token 1" << std::endl;
                nextToken = 1; // Используем безопасный токен
            }

            // Проверяем EOS
            if (nextToken == llama_vocab_eos(llama_model_get_vocab(model))) {
                std::cout << "\nEOS token reached" << std::endl;
                break;
            }

            // Обрабатываем новый токен
            llama_batch batch = llama_batch_init(1, 0, 1);
            memset(&batch, 0, sizeof(batch));

            batch.n_tokens = 1;
            batch.token = (llama_token*)malloc(sizeof(llama_token));
            batch.pos = (llama_pos*)malloc(sizeof(llama_pos));
            batch.logits = (int8_t*)malloc(sizeof(int8_t));

            batch.token[0] = nextToken;
            batch.pos[0] = tokens.size() + i;
            batch.logits[0] = 1;

            int result = llama_decode(ctx, batch);

            free(batch.token);
            free(batch.pos);
            free(batch.logits);

            if (result != 0) {
                std::cout << "Decode error at token " << i << std::endl;
                break;
            }

            // Детокенизируем
            std::string tokenText = detokenize({ nextToken });

            std::cout << "Token " << i << ": '" << tokenText << "' (len=" << tokenText.length() << ")" << std::endl;

            // Фильтруем странные символы
            if (!tokenText.empty() && tokenText.length() < 20) {
                response += tokenText;

                std::cout << tokenText << std::flush; // Прямой вывод в консоль

                if (streamOutput && streamCallback) {
                    streamCallback(tokenText);
                }
            }

            // Более мягкая проверка на окончание
            if (response.length() > 50 && i > 20) { // Минимум 50 символов и 20 токенов
                // Проверяем окончание предложения
                if (response.length() > 2) {
                    std::string last2 = response.substr(response.length() - 2);
                    if (last2 == ". " || last2 == "! " || last2 == "? ") {
                        std::cout << "\nSentence ending detected" << std::endl;
                        break;
                    }
                }

                // Или очень длинный ответ
                if (response.length() > 500) {
                    std::cout << "\nMax length reached" << std::endl;
                    break;
                }
            }
        }

        std::cout << "\nGenerated response: " << response.length() << " characters" << std::endl;

        std::cout << "Full response: '" << response << "'" << std::endl;


        if (response.empty()) {
            return "I understand your question about: " + prompt + ". Could you please be more specific?";
        }

        return response;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return "Error generating response: " + std::string(e.what());
    }
}

// Простая функция семплирования
llama_token LLMInterface::sampleToken(const float* logits, int n_vocab) {
    // Создаем список кандидатов с их логитами

    bool valid = false;
    for (int i = 0; i < n_vocab; i++) {
        if (std::isfinite(logits[i])) {
            valid = true;
            break;
        }
    }

    if (!valid) {
        std::cerr << "Invalid logits detected!" << std::endl;
        return 0; // Возвращаем первый токен
    }

    std::vector<std::pair<float, llama_token>> candidates;

    for (int i = 0; i < n_vocab; i++) {
        candidates.push_back({ logits[i], i });
    }

    // Сортируем по убыванию логитов
    std::sort(candidates.begin(), candidates.end(), std::greater<>());

    // Берем топ-40 (Top-K семплирование)
    const int top_k = std::min(40, n_vocab);
    candidates.resize(top_k);

    // Применяем temperature
    const float temperature = 0.8f;
    for (auto& candidate : candidates) {
        candidate.first /= temperature;
    }

    // Softmax
    float max_logit = candidates[0].first;
    float sum = 0.0f;

    for (auto& candidate : candidates) {
        candidate.first = std::exp(candidate.first - max_logit);
        sum += candidate.first;
    }

    // Нормализуем
    for (auto& candidate : candidates) {
        candidate.first /= sum;
    }

    // Случайный выбор
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(0.0f, 1.0f);

    float r = dis(gen);
    for (const auto& candidate : candidates) {
        r -= candidate.first;
        if (r <= 0) {
            return candidate.second;
        }
    }

    return candidates[0].second;
}

void LLMInterface::stopGeneration() {
    stopRequested = true;
    std::cout << "Generation stop requested" << std::endl;
}

void LLMInterface::resetContext() {
    std::lock_guard<std::mutex> lock(mtx);
    contextData.clear();

    if (ctx) {
        llama_kv_cache_clear(ctx);
    }

    std::cout << "Context and KV cache cleared" << std::endl;
}

std::string LLMInterface::getModelInfo() const {
    if (!loaded || !model) {
        return "Model not loaded";
    }

    std::lock_guard<std::mutex> lock(mtx);

    std::stringstream ss;

    char buf[512] = { 0 };
    llama_model_desc(model, buf, sizeof(buf));

    ss << "Model: " << buf << "\n";
    ss << "Context size: " << llama_model_n_ctx_train(model) << " tokens\n";
    ss << "Parameters: " << std::fixed << std::setprecision(1)
        << (llama_model_n_params(model) / 1e9) << "B\n";

    return ss.str();
}

bool LLMInterface::isLoaded() const {
    return loaded && model && ctx;
}

std::vector<llama_token> LLMInterface::tokenize(const std::string& text, bool addBos) {
    if (!model) {
        return {};
    }

    if (text.empty()) {
        return {};
    }

    try {
        std::vector<llama_token> tokens(text.size() + 10);

        int n = llama_tokenize(
            llama_model_get_vocab(model),
            text.c_str(),
            text.length(),
            tokens.data(),
            tokens.size(),
            addBos,
            false
        );

        if (n < 0) {
            tokens.resize(-n);
            n = llama_tokenize(
                llama_model_get_vocab(model),
                text.c_str(),
                text.length(),
                tokens.data(),
                tokens.size(),
                addBos,
                false
            );
        }

        if (n <= 0) {
            return {};
        }

        tokens.resize(n);
        return tokens;
    }
    catch (const std::exception& e) {
        std::cerr << "Error in tokenization: " << e.what() << std::endl;
        return {};
    }
}

std::string LLMInterface::detokenize(const std::vector<llama_token>& tokens) {
    if (tokens.empty() || !model) {
        return "";
    }

    try {
        std::string result;

        for (const auto& token : tokens) {
            char buf[256] = { 0 };
            int len = llama_token_to_piece(llama_model_get_vocab(model), token, buf, sizeof(buf) - 1, 0, true);

            if (len > 0) {
                buf[len] = '\0';
                result += buf;
            }
        }

        return result;
    }
    catch (const std::exception& e) {
        std::cerr << "Error in detokenization: " << e.what() << std::endl;
        return "";
    }
}
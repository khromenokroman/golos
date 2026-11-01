#pragma once

#include <string>
#include <vector>

struct whisper_context;

/**
 * @brief Транскрибация аудио моделью Whisper (whisper.cpp).
 * Модель загружается один раз, далее возможны повторные вызовы transcribe().
 */
class Transcriber
{
  public:
    explicit Transcriber(const std::string& model_path);
    ~Transcriber();

    Transcriber(const Transcriber&) = delete;
    Transcriber& operator=(const Transcriber&) = delete;

    /**
     * @brief Проверяет, что модель загружена.
     */
    bool valid() const
    {
        return m_ctx != nullptr;
    }

    /**
     * @brief Распознаёт речь в отсчётах 16 кГц моно.
     * @param samples отсчёты в диапазоне [-1.0; 1.0]
     * @param lang код языка ("ru", "en"), пустая строка — автоопределение
     * @return распознанный текст
     */
    std::string transcribe(const std::vector<float>& samples, const std::string& lang);

  private:
    whisper_context* m_ctx = nullptr;
};

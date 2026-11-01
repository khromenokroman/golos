#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

struct ma_device;

/**
 * @brief Захват аудио с микрофона через miniaudio.
 * Записывает монофонический поток 16 кГц в память, по остановке сохраняет в WAV.
 */
class Recorder
{
  public:
    explicit Recorder(std::uint32_t sample_rate = 16000);
    ~Recorder();

    Recorder(const Recorder&) = delete;
    Recorder& operator=(const Recorder&) = delete;

    /**
     * @brief Открывает устройство захвата и начинает запись.
     * @return true при успехе
     */
    bool start();

    /**
     * @brief Останавливает запись и освобождает устройство.
     */
    void stop();

    /**
     * @brief Сохраняет накопленные отсчёты в WAV-файл.
     * @return true при успехе
     */
    bool save_wav(const std::string& path) const;

    /**
     * @brief Возвращает копию накопленных отсчётов (для потоковой транскрибации).
     */
    std::vector<float> snapshot() const;

    std::uint32_t sample_rate() const
    {
        return m_sample_rate;
    }

  private:
    static void capture_callback(ma_device* device, void* output, const void* input, std::uint32_t frame_count);

    mutable std::mutex m_mutex;
    std::vector<float> m_samples;
    ma_device* m_device = nullptr;
    std::uint32_t m_sample_rate;
};

#include "recorder.hpp"

#include "wav.hpp"

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

Recorder::Recorder(std::uint32_t sample_rate) : m_sample_rate(sample_rate)
{
}

Recorder::~Recorder()
{
    stop();
}

bool Recorder::start()
{
    ma_device_config config = ma_device_config_init(ma_device_type_capture);
    config.capture.format = ma_format_f32;
    config.capture.channels = 1;
    config.sampleRate = m_sample_rate;
    config.dataCallback = &Recorder::capture_callback;
    config.pUserData = this;

    m_device = new ma_device;
    if (ma_device_init(nullptr, &config, m_device) != MA_SUCCESS) {
        delete m_device;
        m_device = nullptr;
        return false;
    }
    if (ma_device_start(m_device) != MA_SUCCESS) {
        ma_device_uninit(m_device);
        delete m_device;
        m_device = nullptr;
        return false;
    }
    return true;
}

void Recorder::stop()
{
    if (!m_device) {
        return;
    }
    ma_device_stop(m_device);
    ma_device_uninit(m_device);
    delete m_device;
    m_device = nullptr;
}

bool Recorder::save_wav(const std::string& path) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return wav_write(path, m_samples, m_sample_rate);
}

void Recorder::capture_callback(ma_device* device, void* /*output*/, const void* input, std::uint32_t frame_count)
{
    auto* self = static_cast<Recorder*>(device->pUserData);
    const auto* frames = static_cast<const float*>(input);
    std::lock_guard<std::mutex> lock(self->m_mutex);
    self->m_samples.insert(self->m_samples.end(), frames, frames + frame_count);
}

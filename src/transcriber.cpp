#include "transcriber.hpp"

#include <algorithm>
#include <thread>

#include <whisper.h>

Transcriber::Transcriber(const std::string& model_path)
{
    whisper_log_set([](enum ggml_log_level, const char*, void*) {}, nullptr);
    m_ctx = whisper_init_from_file(model_path.c_str());
}

Transcriber::~Transcriber()
{
    if (m_ctx) {
        whisper_free(m_ctx);
    }
}

std::string Transcriber::transcribe(const std::vector<float>& samples, const std::string& lang,
                                    const std::string& prompt)
{
    if (!m_ctx) {
        return {};
    }

    whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    params.print_progress = false;
    params.print_realtime = false;
    params.print_timestamps = false;
    params.n_threads = std::min<unsigned>(8, std::thread::hardware_concurrency());
    params.no_timestamps = true;
    params.language = lang.empty() ? nullptr : lang.c_str();
    params.initial_prompt = prompt.empty() ? nullptr : prompt.c_str();

    if (whisper_full(m_ctx, params, samples.data(), static_cast<int>(samples.size())) != 0) {
        return {};
    }

    std::string text;
    const int n_segments = whisper_full_n_segments(m_ctx);
    for (int i = 0; i < n_segments; ++i) {
        const char* segment = whisper_full_get_segment_text(m_ctx, i);
        if (segment) {
            text += segment;
        }
    }
    return text;
}

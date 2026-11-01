#include "wav.hpp"

#include <cstdio>
#include <cstring>

namespace
{

void write_u16(std::FILE* f, std::uint16_t v)
{
    std::fwrite(&v, sizeof(v), 1, f);
}

void write_u32(std::FILE* f, std::uint32_t v)
{
    std::fwrite(&v, sizeof(v), 1, f);
}

std::uint16_t read_u16(const std::uint8_t* p)
{
    return static_cast<std::uint16_t>(p[0] | (p[1] << 8));
}

std::uint32_t read_u32(const std::uint8_t* p)
{
    return static_cast<std::uint32_t>(p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24));
}

} // namespace

bool wav_write(const std::string& path, const std::vector<float>& samples, std::uint32_t sample_rate)
{
    std::FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) {
        return false;
    }

    const std::uint32_t data_size = static_cast<std::uint32_t>(samples.size()) * sizeof(std::int16_t);
    const std::uint32_t byte_rate = sample_rate * sizeof(std::int16_t);

    std::fwrite("RIFF", 1, 4, f);
    write_u32(f, 36 + data_size);
    std::fwrite("WAVE", 1, 4, f);

    std::fwrite("fmt ", 1, 4, f);
    write_u32(f, 16); // размер блока fmt
    write_u16(f, 1);  // PCM
    write_u16(f, 1);  // моно
    write_u32(f, sample_rate);
    write_u32(f, byte_rate);
    write_u16(f, sizeof(std::int16_t));     // байт на отсчёт
    write_u16(f, 8 * sizeof(std::int16_t)); // бит на отсчёт

    std::fwrite("data", 1, 4, f);
    write_u32(f, data_size);

    for (float s : samples) {
        float clamped = s < -1.0f ? -1.0f : (s > 1.0f ? 1.0f : s);
        auto pcm = static_cast<std::int16_t>(clamped * 32767.0f);
        std::fwrite(&pcm, sizeof(pcm), 1, f);
    }

    std::fclose(f);
    return true;
}

std::vector<float> wav_read(const std::string& path, std::uint32_t& sample_rate)
{
    sample_rate = 0;
    std::FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) {
        return {};
    }

    std::uint8_t header[12];
    if (std::fread(header, 1, sizeof(header), f) != sizeof(header) || std::memcmp(header, "RIFF", 4) != 0 ||
        std::memcmp(header + 8, "WAVE", 4) != 0) {
        std::fclose(f);
        return {};
    }

    std::vector<float> samples;
    bool have_fmt = false;
    std::uint16_t channels = 0;
    std::uint16_t bits = 0;

    while (true) {
        std::uint8_t chunk[8];
        if (std::fread(chunk, 1, sizeof(chunk), f) != sizeof(chunk)) {
            break;
        }
        const std::uint32_t size = read_u32(chunk + 4);

        if (std::memcmp(chunk, "fmt ", 4) == 0) {
            std::vector<std::uint8_t> buf(size);
            if (std::fread(buf.data(), 1, size, f) != size) {
                break;
            }
            const std::uint16_t format = read_u16(buf.data());
            channels = read_u16(buf.data() + 2);
            sample_rate = read_u32(buf.data() + 4);
            bits = read_u16(buf.data() + 14);
            if ((format != 1 && format != 3) || channels != 1 || (bits != 16 && bits != 32)) {
                std::fclose(f);
                return {};
            }
            have_fmt = true;
        } else if (std::memcmp(chunk, "data", 4) == 0) {
            if (!have_fmt) {
                std::fclose(f);
                return {};
            }
            const std::size_t count = size / (bits / 8);
            std::vector<std::uint8_t> buf(size);
            if (std::fread(buf.data(), 1, size, f) != size) {
                break;
            }
            samples.resize(count);
            if (bits == 16) {
                for (std::size_t i = 0; i < count; ++i) {
                    const auto pcm = static_cast<std::int16_t>(read_u16(buf.data() + i * 2));
                    samples[i] = static_cast<float>(pcm) / 32768.0f;
                }
            } else {
                for (std::size_t i = 0; i < count; ++i) {
                    float v = 0.0f;
                    std::memcpy(&v, buf.data() + i * 4, 4);
                    samples[i] = v;
                }
            }
        } else {
            std::fseek(f, static_cast<long>(size), SEEK_CUR);
        }
    }

    std::fclose(f);
    return samples;
}

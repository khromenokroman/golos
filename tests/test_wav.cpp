#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <vector>

#include "wav.hpp"

namespace
{

TEST(Wav, Roundtrip16Bit)
{
    constexpr std::uint32_t sample_rate = 16000;
    constexpr std::size_t n = 16000;
    std::vector<float> samples(n);
    for (std::size_t i = 0; i < n; ++i) {
        samples[i] =
            0.5f * std::sin(2.0f * 3.14159265f * 440.0f * static_cast<float>(i) / static_cast<float>(sample_rate));
    }

    const char* path = "/tmp/golos_test_roundtrip.wav";
    ASSERT_TRUE(wav_write(path, samples, sample_rate));

    std::uint32_t read_rate = 0;
    const std::vector<float> read = wav_read(path, read_rate);

    ASSERT_EQ(read_rate, sample_rate);
    ASSERT_EQ(read.size(), samples.size());
    for (std::size_t i = 0; i < n; i += 100) {
        EXPECT_NEAR(read[i], samples[i], 1e-4f);
    }
}

TEST(Wav, ClippingOnWrite)
{
    const std::vector<float> samples = {-2.0f, -1.0f, 0.0f, 1.0f, 2.0f};
    const char* path = "/tmp/golos_test_clip.wav";
    ASSERT_TRUE(wav_write(path, samples, 16000));

    std::uint32_t read_rate = 0;
    const std::vector<float> read = wav_read(path, read_rate);
    ASSERT_EQ(read.size(), samples.size());
    EXPECT_NEAR(read[0], -1.0f, 1e-4f);
    EXPECT_NEAR(read[4], 1.0f, 1e-4f);
}

TEST(Wav, ReadMissingFile)
{
    std::uint32_t read_rate = 123;
    const std::vector<float> read = wav_read("/tmp/golos_no_such_file.wav", read_rate);
    EXPECT_TRUE(read.empty());
    EXPECT_EQ(read_rate, 0u);
}

} // namespace

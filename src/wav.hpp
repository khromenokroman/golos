#pragma once

#include <cstdint>
#include <string>
#include <vector>

/**
 * @brief Записывает монофонический PCM WAV (16 бит) в файл.
 * @param path путь к файлу
 * @param samples отсчёты, нормализованные к диапазону [-1.0; 1.0]
 * @param sample_rate частота дискретизации
 * @return true при успехе
 */
bool wav_write(const std::string& path, const std::vector<float>& samples, std::uint32_t sample_rate);

/**
 * @brief Читает монофонический PCM WAV (16 или 32 бит float) из файла.
 * @param path путь к файлу
 * @param sample_rate сюда записывается частота дискретизации
 * @return отсчёты, нормализованные к диапазону [-1.0; 1.0]; пустой вектор при ошибке
 */
std::vector<float> wav_read(const std::string& path, std::uint32_t& sample_rate);

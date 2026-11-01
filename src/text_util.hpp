#pragma once

#include <algorithm>
#include <string>
#include <utility>

/**
 * @brief Возвращает копию строки без пробелов, табуляций и переводов строк по краям.
 */
inline std::string trim(const std::string& s)
{
    const auto first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }
    const auto last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, last - first + 1);
}

/**
 * @brief Число символов (не байт) в UTF-8 строке.
 */
inline std::size_t utf8_length(const std::string& s)
{
    std::size_t n = 0;
    for (unsigned char c : s) {
        if ((c & 0xC0) != 0x80) {
            ++n;
        }
    }
    return n;
}

/**
 * @brief Длина UTF-8 символа по лидирующему байту.
 */
inline std::size_t utf8_char_len(char lead)
{
    const auto c = static_cast<unsigned char>(lead);
    if ((c & 0x80) == 0) {
        return 1;
    }
    if ((c & 0xE0) == 0xC0) {
        return 2;
    }
    if ((c & 0xF0) == 0xE0) {
        return 3;
    }
    return 4;
}

/**
 * @brief Вычисляет правку для потоковой печати: сколько backspace отправить
 * и какой суффикс напечатать, чтобы напечатанный текст typed стал равен full.
 * Сравнение посимвольное (UTF-8), backspace считаются в символах —
 * dotool стирает по одному символу.
 */
inline std::pair<std::size_t, std::string> typing_diff(const std::string& typed, const std::string& full)
{
    std::size_t pos_t = 0;
    std::size_t pos_f = 0;
    std::size_t common_chars = 0;
    while (pos_t < typed.size() && pos_f < full.size()) {
        const std::size_t len_t = utf8_char_len(typed[pos_t]);
        const std::size_t len_f = utf8_char_len(full[pos_f]);
        if (len_t != len_f || typed.compare(pos_t, len_t, full, pos_f, len_f) != 0) {
            break;
        }
        pos_t += len_t;
        pos_f += len_f;
        ++common_chars;
    }
    return {utf8_length(typed) - common_chars, full.substr(pos_f)};
}

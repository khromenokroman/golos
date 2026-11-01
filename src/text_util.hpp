#pragma once

#include <string>

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

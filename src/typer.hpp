#pragma once

#include <string>

/**
 * @brief Печатает текст в активное окно ввода.
 * Бэкенд выбирается так: переменная окружения GOLOS_TYPER, если задана;
 * иначе wtype при Wayland-сессии, иначе xdotool.
 * @param text строка UTF-8
 * @return true при успехе
 */
bool type_text(const std::string& text);

#pragma once

#include <cstddef>
#include <string>

/**
 * @brief Печатает текст в активное окно ввода.
 * Бэкенд выбирается так: переменная окружения GOLOS_TYPER, если задана;
 * иначе wtype при Wayland-сессии, иначе xdotool.
 * @param text строка UTF-8
 * @return true при успехе
 */
bool type_text(const std::string& text);

/**
 * @brief Потоковая правка напечатанного текста: стирает backspaces символов
 * и печатает суффикс. Оба действия выполняются одним запуском dotool.
 * @param backbacks сколько раз нажать BackSpace
 * @param text суффикс для печати
 * @return true при успехе
 */
bool type_correction(std::size_t backspaces, const std::string& text);

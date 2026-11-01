# golos

Диктовка речи в текст прямо в активное окно. Нажал горячую клавишу, говоришь, нажал ещё раз — текст напечатан.

Работает локально (офлайн) на базе модели Whisper через [whisper.cpp]. Языки: русский, английский и ещё ~90.

## Сборка

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Зависимости: cmake, g++ (C++17), git. whisper.cpp и miniaudio подтягиваются автоматически через FetchContent.

## Установка

```bash
./scripts/download_model.sh        # модель small (~460 МБ), или tiny|base|medium|large
sudo apt install wtype             # Wayland — печать в окна
sudo apt install xdotool           # альтернатива для X11
sudo install -m 755 build/golos /usr/local/bin/golos
./scripts/setup_hotkey.sh          # Ctrl+Alt+Space -> golos toggle
```

## Использование

Нажмите `Ctrl+Alt+Space`, скажите фразу, нажмите `Ctrl+Alt+Space` ещё раз — текст появится в текущем поле ввода.

Команды:

```
golos toggle            начать/остановить диктовку (для хоткея)
golos status            состояние: recording / idle
golos end --no-type     только вывести текст в stdout, без печати
golos transcribe f.wav  распознать готовый файл
golos --lang en toggle  английский (по умолчанию ru)
```

## Как это устроено

1. `golos begin` форкает фоновый процесс записи (`golos record`), который пишет микрофон 16 кГц в WAV. PID хранится в `$XDG_RUNTIME_DIR/golos.pid`.
2. `golos end` шлёт SIGTERM, ждёт завершения, читает WAV, прогоняет через whisper.cpp и печатает результат через `wtype` (Wayland) или `xdotool` (X11).
3. Горячая клавиша — штатная пользовательская комбинация GNOME, срабатывает на любой раскладке. Постоянного демона нет: состояние живёт в pid-файле, модель грузится только на время распознавания.

Лог записи: `~/.local/state/golos/golos.log`. Модель: `~/.local/share/golos/ggml-small.bin`.

[whisper.cpp]: https://github.com/ggerganov/whisper.cpp

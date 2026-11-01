# golos

Диктовка речи в текст прямо в активное окно. Нажал горячую клавишу, говоришь, нажал ещё раз — текст напечатан.

Работает локально (офлайн) на базе модели Whisper через [whisper.cpp]. Языки: русский, английский и ещё ~90.

## Установка

Готовый пакет для Debian 13 (amd64) — на [странице релизов](https://github.com/khromenokroman/golos/releases):

```bash
sudo apt install ./golos_0.1.1_amd64.deb
/usr/share/golos/download_model.sh    # модель small (~460 МБ), или tiny|base|medium|large
/usr/share/golos/setup_hotkey.sh      # Ctrl+Alt+Space -> golos toggle
```

Из пакета зависимости подтянутся сами: `libgomp1`, а для печати в окна —
`wtype` (Wayland) или `xdotool` (X11).

## Сборка из исходников

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Зависимости: cmake, g++ (C++17), git. whisper.cpp и miniaudio подтягиваются
автоматически через FetchContent и линкуются статически.

Запуск без установки:

```bash
./build/golos toggle
```

Установка вручную:

```bash
./scripts/download_model.sh
sudo apt install wtype              # Wayland; для X11 — xdotool
sudo install -m 755 build/golos /usr/local/bin/golos
./scripts/setup_hotkey.sh
```

### Печать под GNOME Wayland

Печать выполняется через внешний бэкенд (переменная `GOLOS_TYPER`
перекрывает выбор): **dotool** (uinput, работает везде, рекомендуется),
`wtype` (wlroots-композиторы), `xdotool` (X11 и XWayland-окна).

GNOME Wayland не поддерживает ни протокол `wtype`, ни XTEST в нативные
окна, поэтому нужен dotool:

```bash
git clone https://git.sr.ht/~geb/dotool && cd dotool
./build.sh && sudo ./build.sh install          # бинарник + udev-правило
sudo usermod -aG input "$USER"                 # доступ к /dev/uinput
# перелогиниться, затем проверить автоматическим e2e-тестом:
tests/typing_e2e.sh
```

Сборка пакета: `cpack -G DEB` в каталоге сборки, результат —
`golos_<версия>_amd64.deb`.

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

## CI

GitHub Actions (`.github/workflows/release.yml`), цель — Debian 13 (trixie):

- `build`: сборка и тесты в контейнере `debian:trixie`, пакет `cpack -G DEB`,
  выгрузка артефакта `golos-debian13`
- `install`: установка пакета в чистом `debian:trixie`, проверка `golos status`, удаление
- `release` (только тег `v*`): проверка совпадения тега и версии в
  CMakeLists.txt, публикация .deb в релизах GitHub

## Как это устроено

1. `golos begin` форкает фоновый процесс записи (`golos record`), который пишет микрофон 16 кГц в WAV. PID хранится в `$XDG_RUNTIME_DIR/golos.pid`.
2. `golos end` шлёт SIGTERM, ждёт завершения, читает WAV, прогоняет через whisper.cpp и печатает результат через `wtype` (Wayland) или `xdotool` (X11).
3. Горячая клавиша — штатная пользовательская комбинация GNOME, срабатывает на любой раскладке. Постоянного демона нет: состояние живёт в pid-файле, модель грузится только на время распознавания.

Лог записи: `~/.local/state/golos/golos.log`. Модель: `~/.local/share/golos/ggml-small.bin`.

## Тесты

googletest, цель `golos_tests`: roundtrip WAV, ограничение амплитуды,
чтение битых файлов, граничные случаи `trim`.

[whisper.cpp]: https://github.com/ggerganov/whisper.cpp

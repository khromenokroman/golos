#!/bin/bash
# E2E-тест печати: отправляет текст через dotool в окно-приёмник (Gtk.Entry)
# и проверяет, что текст дошёл. Требует dotool и доступа к /dev/uinput
# (группа input). Запуск: tests/typing_e2e.sh
set -uo pipefail

cd "$(dirname "$0")/.."

RESULT=/tmp/golos_typing_e2e.txt
MARKER="Голосовой ввод ёжик 777"
rm -f "$RESULT"

if ! command -v dotool > /dev/null; then
    echo "FAIL: dotool не установлен"
    exit 1
fi

python3 tests/atspi_receiver.py "$RESULT" > /dev/null 2>&1 &
recv=$!
sleep 2

echo "type $MARKER" | DOTOOL_XKB_LAYOUT="${DOTOOL_XKB_LAYOUT:-ru,us}" dotool
sleep 4

kill "$recv" 2>/dev/null
wait "$recv" 2>/dev/null

actual=$(cat "$RESULT" 2>/dev/null)
if [ "$actual" = "$MARKER" ]; then
    echo "OK: dotool доставил текст '$actual'"
    exit 0
fi
echo "FAIL: ожидалось '$MARKER', получено '$actual'"
exit 1

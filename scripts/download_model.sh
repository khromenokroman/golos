#!/bin/bash
# Скачивает модель Whisper для golos.
# Использование: ./download_model.sh [small|base|medium|large]   (по умолчанию small)
set -euo pipefail

MODEL="${1:-small}"
DATA_DIR="${XDG_DATA_HOME:-$HOME/.local/share}/golos"
mkdir -p "$DATA_DIR"

case "$MODEL" in
    tiny)    FILE=ggml-tiny.bin;    URL="https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-tiny.bin" ;;
    base)    FILE=ggml-base.bin;    URL="https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.bin" ;;
    small)   FILE=ggml-small.bin;   URL="https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-small.bin" ;;
    medium)  FILE=ggml-medium.bin;  URL="https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-medium.bin" ;;
    large)   FILE=ggml-large-v3.bin; URL="https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-large-v3.bin" ;;
    *) echo "Неизвестная модель: $MODEL (tiny|base|small|medium|large)" >&2; exit 1 ;;
esac

if [ -f "$DATA_DIR/$FILE" ]; then
    echo "Модель уже скачана: $DATA_DIR/$FILE"
    exit 0
fi

echo "Скачивание $FILE в $DATA_DIR ..."
wget -q --show-progress -O "$DATA_DIR/$FILE" "$URL"
echo "Готово: $DATA_DIR/$FILE"

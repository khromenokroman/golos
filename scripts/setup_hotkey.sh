#!/bin/bash
# Регистрирует пользовательскую горячую клавишу GNOME, вызывающую golos toggle.
# Использование: ./setup_hotkey.sh [команда golos]   (по умолчанию: golos toggle из PATH)
set -euo pipefail

GOLOS_CMD="${1:-golos toggle}"
KEY="${GOLOS_HOTKEY:-<Ctrl><Alt>space}"

SCHEMA=org.gnome.settings-daemon.plugins.media-keys
BASE="/org/gnome/settings-daemon/plugins/media-keys/custom-keybindings"

# ищем свободный слот custom0..custom99
current=$(gsettings get "$SCHEMA" custom-keybindings)
idx=0
while echo "$current" | grep -q "custom$idx"; do idx=$((idx+1)); done

path="$BASE/custom$idx/"

gsettings set "$SCHEMA" custom-keybindings "${current%]*}, '$path']"
gsettings set "$SCHEMA.custom-keybinding:$path" name 'Golos dictation'
gsettings set "$SCHEMA.custom-keybinding:$path" command "$GOLOS_CMD"
gsettings set "$SCHEMA.custom-keybinding:$path" binding "$KEY"

echo "Готово: $KEY запускает '$GOLOS_CMD'"
echo "Поменять клавишу: GOLOS_HOTKEY='<Ctrl><Alt>d' ./setup_hotkey.sh"

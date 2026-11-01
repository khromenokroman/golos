#!/bin/bash
# Регистрирует пользовательскую горячую клавишу GNOME, вызывающую golos toggle.
# Использование: ./setup_hotkey.sh [команда golos]   (по умолчанию: golos toggle из PATH)
set -euo pipefail

GOLOS_CMD="${1:-golos toggle}"
KEY="${GOLOS_HOTKEY:-<Ctrl><Alt>space}"

SCHEMA=org.gnome.settings-daemon.plugins.media-keys
BASE="/org/gnome/settings-daemon/plugins/media-keys/custom-keybindings"

# собираем существующие привязки в валидный gvariant-массив
items=$(gsettings get "$SCHEMA" custom-keybindings | grep -o "'[^']*'" || true)

list="["
first=1
while IFS= read -r item; do
    [ -n "$item" ] || continue
    [ "$first" -eq 0 ] && list+=", "
    list+="$item"
    first=0
done <<< "$items"

# ищем свободный слот custom0..custom99
idx=0
while grep -q "custom$idx/" <<< "$list"; do idx=$((idx+1)); done
path="$BASE/custom$idx/"

[ "$first" -eq 0 ] && list+=", "
list+="'$path']"

gsettings set "$SCHEMA" custom-keybindings "$list"
gsettings set "$SCHEMA.custom-keybinding:$path" name 'Golos dictation'
gsettings set "$SCHEMA.custom-keybinding:$path" command "$GOLOS_CMD"
gsettings set "$SCHEMA.custom-keybinding:$path" binding "$KEY"

echo "Готово: $KEY запускает '$GOLOS_CMD' (слот custom$idx)"
echo "Поменять клавишу: GOLOS_HOTKEY='<Ctrl><Alt>d' ./setup_hotkey.sh"

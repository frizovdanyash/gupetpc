#!/usr/bin/env bash
set -e

echo "=== Удаление GUPet ==="

if [ "$EUID" -eq 0 ]; then
    rm -f /usr/local/bin/gupet
    rm -f /usr/share/applications/gupet.desktop
    rm -f /usr/share/icons/hicolor/128x128/apps/gupet.png
else
    rm -f "$HOME/.local/bin/gupet"
    rm -f "$HOME/.local/share/applications/gupet.desktop"
    rm -f "$HOME/.local/share/icons/hicolor/128x128/apps/gupet.png"
    rm -f "$HOME/.config/autostart/gupet.desktop"
fi

echo "GUPet успешно удалён из системы."

#!/usr/bin/env bash
set -e

echo "=== Установка GUPet для Linux ==="

# Check binary exists, compile if needed
if [ ! -f "bin/gupet" ]; then
    echo "Сборка исполняемого файла bin/gupet..."
    make linux
fi

# Detect install prefix
if [ "$EUID" -eq 0 ]; then
    BIN_DIR="/usr/local/bin"
    APP_DIR="/usr/share/applications"
    ICON_DIR="/usr/share/icons/hicolor/128x128/apps"
else
    BIN_DIR="$HOME/.local/bin"
    APP_DIR="$HOME/.local/share/applications"
    ICON_DIR="$HOME/.local/share/icons/hicolor/128x128/apps"
fi

mkdir -p "$BIN_DIR"
mkdir -p "$APP_DIR"
mkdir -p "$ICON_DIR"

echo "Установка исполняемого файла в $BIN_DIR/gupet..."
cp bin/gupet "$BIN_DIR/gupet"
chmod +x "$BIN_DIR/gupet"

echo "Установка иконки в $ICON_DIR/gupet.png..."
cp res/gupet.png "$ICON_DIR/gupet.png"

echo "Установка ярлыка в $APP_DIR/gupet.desktop..."
cp gupet.desktop "$APP_DIR/gupet.desktop"

# Install bundled skins
SKINS_DEST="$HOME/.config/gupet/skins"
mkdir -p "$SKINS_DEST"
if [ -d "skins" ]; then
    echo "Копирование встроенных скинов в $SKINS_DEST..."
    cp -r skins/* "$SKINS_DEST/" 2>/dev/null || true
fi

# Update desktop & icon caches if tools exist
if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database "$APP_DIR" 2>/dev/null || true
fi
if command -v gtk-update-icon-cache >/dev/null 2>&1; then
    gtk-update-icon-cache -f -t "$(dirname "$(dirname "$ICON_DIR")")" 2>/dev/null || true
fi

echo ""
echo "=== Установка успешно завершена! ==="
echo "Вы можете запустить питомца командой: gupet"
echo "Или найти GUPet в меню приложений."

#!/bin/bash
set -e

if [[ "$(uname)" != "Darwin" ]]; then
    echo "This script can only be run on macOS!"
    exit 1
fi

# 1. Определение путей к Qt6 (через $QT_ROOT_DIR на CI или через Homebrew локально)
if [ -z "$CMAKE_PREFIX_PATH" ]; then
    if [ -n "$QT_ROOT_DIR" ]; then
        export CMAKE_PREFIX_PATH="$QT_ROOT_DIR"
    elif command -v brew &> /dev/null; then
        BREW_PREFIX=$(brew --prefix)
        export PATH="$BREW_PREFIX/opt/qt/bin:$BREW_PREFIX/opt/qt6/bin:$PATH"
        export CMAKE_PREFIX_PATH="$BREW_PREFIX/opt/qt6:$BREW_PREFIX/opt/qt"
    fi
fi

# 2. Очистка старой сборки
rm -rf build-macos dmg_root Studio-3D-Viewer.dmg
mkdir -p build-macos

# 3. Конфигурация и компиляция Universal Binary (Intel + Apple Silicon)
cmake -B build-macos \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET="12.0"

cmake --build build-macos -j$(sysctl -n hw.ncpu)

# 4. Развертывание зависимостей Qt внутрь бандла .app
if command -v macdeployqt &> /dev/null; then
    macdeployqt "build-macos/Studio 3D Viewer.app" 2>&1 | grep -v -E "Cannot resolve rpath|using QList|using QSet" || true
fi

# Принудительная локальная подпись компонентов
codesign --force --deep --sign - "build-macos/Studio 3D Viewer.app"

# 5. Упаковка в установщик DMG
if command -v create-dmg &> /dev/null; then
    mkdir -p dmg_root
    cp -R "build-macos/Studio 3D Viewer.app" dmg_root/

    create-dmg \
      --volname "Studio 3D Viewer Installer" \
      --window-pos 200 120 \
      --window-size 600 450 \
      --icon-size 100 \
      --icon "Studio 3D Viewer.app" 150 150 \
      --hide-extension "Studio 3D Viewer.app" \
      --app-drop-link 450 150 \
      Studio-3D-Viewer.dmg \
      dmg_root/ > /dev/null 2>&1

    rm -rf dmg_root
fi

echo "
Done!
"

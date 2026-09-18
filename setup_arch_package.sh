#!/bin/bash
set -e

if [[ "$(uname)" != "Linux" ]] || ! command -v pacman > /dev/null; then
    echo "This script can only be run on Arch-based Linux!"
    exit 1
fi

# Запрос sudo в самом начале
sudo echo > /dev/null

# 1. Сборка бинарника, если еще не собран
if [[ ! -f build/Studio-3D-Viewer ]]; then
    bash compile.sh
fi


mkdir studio-3d-viewer_pkg
cp build/Studio-3D-Viewer studio-3d-viewer_pkg
cp assets/icon.png studio-3d-viewer_pkg/icon.png
cd studio-3d-viewer_pkg

export PACKAGER="Arta <arta@gmail.com>"

# 3. Генерация PKGBUILD
cat > PKGBUILD << 'EOF'
# Maintainer: Arta <arta@gmail.com>
pkgname=studio-3d-viewer
pkgver=1.0.0
pkgrel=1
pkgdesc="Professional, lightweight, and mathematically strict 3D model inspection tool"
arch=("x86_64")
url="https://github.com/Arta48/Studio-3D-Viewer"
depends=(
    qt6-base
    gcc-libs
    glibc
    hicolor-icon-theme
)
makedepends=(
    imagemagick
)
optdepends=(
    "qt6-wayland: native Wayland support"
)
source=(
    "Studio-3D-Viewer"
    "icon.png"
)
sha256sums=(
    "SKIP"
    "SKIP"
)

prepare() {
    cd "${srcdir}"

    _icon="${pkgname//-/}" # studio3dviewer

    # Generate icons of different sizes
    sizes=("16" "24" "32" "48" "64" "128" "256")
    for size in "${sizes[@]}"; do
        magick icon.png -resize "${size}x${size}" -gravity center -background transparent -extent "${size}x${size}" "icon-${size}.png"
    done

    # Creating a desktop file
    cat > "${pkgname}.desktop" <<_DESKTOP_EOF
[Desktop Entry]
Type=Application
Name=Studio 3D Viewer
Name[ru]=Studio 3D Viewer
Comment=Professional, lightweight, and mathematically strict 3D model inspection tool
Comment[ru]=Профессиональный, легковесный и математически строгий инструмент для инспекции 3D-моделей
Exec=studio-3d-viewer %F
Icon=${_icon}
Terminal=false
Categories=Graphics;3DGraphics;Viewer;Development;
MimeType=model/obj;model/gltf+json;model/gltf-binary;
StartupWMClass=${pkgname}
Keywords=3d;viewer;obj;gltf;glb;pbr;model;
_DESKTOP_EOF
}

package() {
    cd "${srcdir}"

    _icon="${pkgname//-/}" # studio3dviewer

    # Installing the binary in /opt
    install -Dm755 Studio-3D-Viewer "${pkgdir}/opt/${pkgname}/Studio-3D-Viewer"

    # Creating a symbolic link in /usr/bin
    install -d "${pkgdir}/usr/bin"
    ln -s /opt/${pkgname}/Studio-3D-Viewer "${pkgdir}/usr/bin/${pkgname}"

    # Installing Icons
    sizes=("16" "24" "32" "48" "64" "128" "256")
    for size in "${sizes[@]}"; do
        install -Dm644 "icon-${size}.png" "${pkgdir}/usr/share/icons/hicolor/${size}x${size}/apps/${_icon}.png"
    done

    # Installing a desktop file
    install -Dm644 "${pkgname}.desktop" "${pkgdir}/usr/share/applications/${pkgname}.desktop"
}
EOF

# 4. Сборка и установка в систему
makepkg -si --skipinteg --noconfirm

cd .. && rm -rf studio-3d-viewer_pkg

echo "
Done!
"

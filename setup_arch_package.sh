#!/bin/bash

if [[ "$(uname)" != "Linux" ]] || ! command -v pacman > /dev/null; then
    echo "This script can only be run on Arch-based Linux!"
    exit 1
fi


# sudo at the beginning
sudo echo > /dev/null


if [[ ! -f build/Studio3DViewer ]]; then
    sh compile.sh || exit 1
fi


mkdir studio-3d-viewer_pkg
cp build/Studio3DViewer studio-3d-viewer_pkg
cp assets/icon.png studio-3d-viewer_pkg/icon.png
cd studio-3d-viewer_pkg


# Info about Packager
export PACKAGER="Arta <arta@gmail.com>"


echo '# Maintainer: Arta <arta@gmail.com>
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
    "Studio3DViewer"
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
    cat > "${pkgname}.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=Studio 3D Viewer
Name[ru]=Студийный 3D-просмотрщик
Comment=Professional, lightweight, and mathematically strict 3D model inspection tool
Comment[ru]=Профессиональный, легковесный и математически строгий инструмент для инспекции 3D-моделей
Exec=studio-3d-viewer %F
Icon=${_icon}
Terminal=false
Categories=Graphics;3DGraphics;Viewer;Development;
MimeType=model/obj;model/gltf+json;model/gltf-binary;
StartupWMClass=Studio3DViewer
Keywords=3d;viewer;obj;gltf;glb;pbr;model;
EOF
}

package() {
    cd "${srcdir}"

    _icon="${pkgname//-/}" # studio3dviewer

    # Installing the binary in /opt
    install -Dm755 Studio3DViewer "${pkgdir}/opt/${pkgname}/Studio3DViewer"

    # Creating a symbolic link in /usr/bin
    install -d "${pkgdir}/usr/bin"
    ln -s /opt/${pkgname}/Studio3DViewer "${pkgdir}/usr/bin/${pkgname}"

    # Installing Icons
    sizes=("16" "24" "32" "48" "64" "128" "256")
    for size in "${sizes[@]}"; do
        install -Dm644 "icon-${size}.png" "${pkgdir}/usr/share/icons/hicolor/${size}x${size}/apps/${_icon}.png"
    done

    # Installing a desktop file
    install -Dm644 "${pkgname}.desktop" "${pkgdir}/usr/share/applications/${pkgname}.desktop"
}' > PKGBUILD


makepkg -si --skipinteg --noconfirm


cd .. && rm -rf studio-3d-viewer_pkg


# Status output
echo "

Done!"

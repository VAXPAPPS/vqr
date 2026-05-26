#!/bin/bash

# Define variables
APP_NAME="vqr"
VERSION="1.0.0"
ARCH="amd64"
DEB_DIR="build_deb/${APP_NAME}_${VERSION}_${ARCH}"

# Create directories
echo "Creating directory structure..."
mkdir -p "$DEB_DIR/DEBIAN"
mkdir -p "$DEB_DIR/usr/bin"
mkdir -p "$DEB_DIR/usr/share/applications"

# Compile application
echo "Building binary..."
make

# Copy files
echo "Copying files..."
cp "$APP_NAME" "$DEB_DIR/usr/bin/"
cp "${APP_NAME}.desktop" "$DEB_DIR/usr/share/applications/"

# Create control file
echo "Generating DEBIAN/control..."
cat <<EOF > "$DEB_DIR/DEBIAN/control"
Package: $APP_NAME
Version: $VERSION
Section: utils
Priority: optional
Architecture: $ARCH
Depends: libgtk-3-0, libqrencode4, libzbar0, libgstreamer1.0-0, libgstreamer-plugins-base1.0-0, gstreamer1.0-plugins-good, gstreamer1.0-plugins-base, libcairo2
Maintainer: Aether OS Developer
Description: Professional QR Code Scanner and Creator.
 A fast, clean architecture based application to generate and scan QR codes from images or webcams.
EOF

# Build debian package
echo "Building Debian Package..."
dpkg-deb --build "$DEB_DIR"

echo "Done! Package is ready: build_deb/${APP_NAME}_${VERSION}_${ARCH}.deb"

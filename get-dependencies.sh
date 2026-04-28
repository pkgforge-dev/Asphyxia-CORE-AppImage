#!/bin/sh

set -eu

ARCH=$(uname -m)

echo "Installing package dependencies..."
echo "---------------------------------------------------------------"
# pacman -Syu --noconfirm PACKAGESHERE

echo "Installing debloated packages..."
echo "---------------------------------------------------------------"
get-debloated-pkgs --add-common --prefer-nano

# Comment this out if you need an AUR package
#make-aur-package PACKAGENAME
echo "Getting binary..."
echo "---------------------------------------------------------------"
mkdir -p ./AppDir/bin
cd ./AppDir/bin

case "$ARCH" in
	x86_64)  farch=x64;;
	aarch64) farch=arm64;;
esac

link=https://github.com/asphyxia-core/core/releases/latest/download/asphyxia-core-linux-$farch.zip

if ! wget --retry-connrefused --tries=30 "$link" -O /tmp/temp.zip 2>/tmp/download.log; then
	cat /tmp/download.log
	exit 1
fi

unzip /tmp/temp.zip

awk -F'/' '/Location:/{print $(NF-1); exit}' /tmp/download.log > ~/version

# asphyxia-core gets broken by strip
# save it to /tmp to restore it later
cp -v ./asphyxia-core /tmp

# add plugins
git clone --depth 1 https://github.com/asphyxia-core/plugins ./_plugins
cp -rv ./_plugins/* ./plugins
rm -rf ./_plugins


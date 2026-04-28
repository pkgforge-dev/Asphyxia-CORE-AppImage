#!/bin/sh

set -eu

ARCH=$(uname -m)
export ARCH
export OUTPATH=./dist
export ADD_HOOKS="self-updater.hook"
export UPINFO="gh-releases-zsync|${GITHUB_REPOSITORY%/*}|${GITHUB_REPOSITORY#*/}|latest|*$ARCH.AppImage.zsync"
export ICON=https://asphyxia-core.github.io/img/core-logo.png
export DESKTOP=DUMMY
export MAIN_BIN=asphyxia-core

# Deploy dependencies
quick-sharun ./AppDir/bin/*

# restore the binary as it gets broken by strip
cp -v /tmp/asphyxia-core ./AppDir/shared/bin

# Turn AppDir into AppImage
quick-sharun --make-appimage

# Test the app for 12 seconds, if the test fails due to the app
# having issues running in the CI use --simple-test instead
quick-sharun --simple-test ./dist/*.AppImage

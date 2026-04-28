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

# Build the preload shim that redirects /proc/self/exe reads to the restored
# binary while leaving execve untouched.
mkdir -p ./AppDir/lib
CC=${CC:-}
if [ -z "$CC" ]; then
	if command -v cc >/dev/null 2>&1; then
		CC=cc
	elif command -v gcc >/dev/null 2>&1; then
		CC=gcc
	elif command -v clang >/dev/null 2>&1; then
		CC=clang
	else
		echo "No C compiler found for fix-proc-exe.so" >&2
		exit 1
	fi
fi

"$CC" -shared -fPIC -O2 -Wall -Wextra -o ./AppDir/lib/fix-proc-exe.so ./fix-proc-exe.c -ldl

if [ ! -f ./AppDir/.preload ]; then
	touch ./AppDir/.preload
fi

if ! grep -qxF 'fix-proc-exe.so' ./AppDir/.preload; then
	printf '%s\n' fix-proc-exe.so >> ./AppDir/.preload
fi

# Turn AppDir into AppImage
quick-sharun --make-appimage

# Test the app for 12 seconds, if the test fails due to the app
# having issues running in the CI use --simple-test instead
quick-sharun --simple-test ./dist/*.AppImage

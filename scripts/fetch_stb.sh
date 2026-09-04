#!/usr/bin/env sh
set -eu

STB_DIR="external/stb"
mkdir -p "$STB_DIR"

fetch() {
    name="$1"
    url="https://raw.githubusercontent.com/nothings/stb/master/$name"

    if [ -f "$STB_DIR/$name" ]; then
        printf '%s already exists\n' "$STB_DIR/$name"
        return
    fi

    printf 'Downloading %s...\n' "$name"

    if command -v curl >/dev/null 2>&1; then
        curl -L --fail "$url" -o "$STB_DIR/$name"
    elif command -v wget >/dev/null 2>&1; then
        wget -O "$STB_DIR/$name" "$url"
    else
        echo "error: curl or wget is required to download STB" >&2
        exit 1
    fi
}

fetch stb_image.h
fetch stb_image_write.h

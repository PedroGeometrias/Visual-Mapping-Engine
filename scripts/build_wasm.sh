#!/usr/bin/env bash
set -euo pipefail

EMXX="${EMXX:-em++}"
EIGEN_INCLUDE="${EIGEN_INCLUDE:-/usr/include/eigen3}"

if ! command -v "$EMXX" >/dev/null 2>&1; then
    echo "em++ was not found. Activate the Emscripten SDK first." >&2
    exit 1
fi

if [ ! -f "$EIGEN_INCLUDE/Eigen/Dense" ]; then
    echo "Eigen was not found at: $EIGEN_INCLUDE" >&2
    echo "Set EIGEN_INCLUDE to your Eigen include directory." >&2
    exit 1
fi

mapfile -t SOURCES < <(find src -name '*.cpp' ! -path 'src/debug/*' | sort)

mkdir -p web

"$EMXX" \
    -std=c++17 \
    -O2 \
    -Wall \
    -Wextra \
    -Iinclude \
    -isystem external/stb \
    -isystem "$EIGEN_INCLUDE" \
    "${SOURCES[@]}" \
    --no-entry \
    -sWASM=1 \
    -sALLOW_MEMORY_GROWTH=1 \
    -sMODULARIZE=1 \
    -sEXPORT_NAME=VisualMappingEngine \
    -sENVIRONMENT=web \
    -sEXPORTED_FUNCTIONS='["_malloc","_free","_vme_reset","_vme_add_image","_vme_build_panorama","_vme_panorama_width","_vme_panorama_height","_vme_panorama_size","_vme_panorama_pixels","_vme_last_error"]' \
    -sEXPORTED_RUNTIME_METHODS='["ccall"]' \
    -o web/engine.js

echo "Built web/engine.js and web/engine.wasm"

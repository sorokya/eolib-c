#!/usr/bin/env bash
# Build eolib as a WASM module using Emscripten.
#
# Prerequisites:
#   - Emscripten SDK installed and activated (source emsdk_env.sh)
#   - Run from the repository root OR from the wasm/ directory
#
# Usage:
#   cd wasm && ./build.sh
#   # Outputs: wasm/eolib.js  wasm/eolib.wasm

set -euo pipefail

# Resolve paths regardless of where the script is called from
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
OUT_DIR="$SCRIPT_DIR"

echo "Building eolib WASM module..."
echo "  Repo root : $REPO_ROOT"
echo "  Output    : $OUT_DIR"

if ! command -v emcc &>/dev/null; then
    echo ""
    echo "Error: emcc not found."
    echo "Please install and activate the Emscripten SDK:"
    echo "  git clone https://github.com/emscripten-core/emsdk.git"
    echo "  cd emsdk && ./emsdk install latest && ./emsdk activate latest"
    echo "  source ./emsdk_env.sh"
    exit 1
fi

echo "  emcc      : $(command -v emcc)"
echo "  version   : $(emcc --version | head -1)"

# Exported C functions
EXPORTED_FUNCTIONS='[
  "_malloc",
  "_free",
  "_wasm_encode_number_buf",
  "_wasm_decode_number_bytes",
  "_wasm_server_verification_hash",
  "_wasm_generate_swap_multiple",
  "_wasm_generate_sequence_start",
  "_wasm_sequence_start_from_init",
  "_wasm_sequence_start_from_ping",
  "_wasm_encrypt_packet",
  "_wasm_decrypt_packet",
  "_wasm_encode_string",
  "_wasm_decode_string",
  "_wasm_string_to_windows_1252",
  "_wasm_packet_roundtrip",
  "_wasm_get_roundtrip_len"
]'

emcc \
    "$REPO_ROOT/src/data.c" \
    "$REPO_ROOT/src/encrypt.c" \
    "$REPO_ROOT/src/sequencer.c" \
    "$REPO_ROOT/src/protocol.c" \
    "$SCRIPT_DIR/wasm_api.c" \
    "$SCRIPT_DIR/wasm_protocol_dispatch.c" \
    -I "$REPO_ROOT/include" \
    -std=c11 \
    -D_POSIX_C_SOURCE=200809L \
    -O2 \
    -s WASM=1 \
    -s MODULARIZE=1 \
    -s EXPORT_NAME='EolibModule' \
    -s EXPORTED_FUNCTIONS="$(echo "$EXPORTED_FUNCTIONS" | tr -d '\n ')" \
    -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap","getValue","setValue","HEAPU8","stringToNewUTF8","UTF8ToString"]' \
    -s ALLOW_MEMORY_GROWTH=1 \
    -gsource-map=inline \
    -o "$OUT_DIR/eolib.js"

echo ""
echo "Build successful!"
echo "  $OUT_DIR/eolib.js"
echo "  $OUT_DIR/eolib.wasm"
echo "  $OUT_DIR/eolib.wasm.map"
echo ""
echo "Serve the wasm/ directory with any static file server, e.g.:"
echo "  cd wasm && python3 -m http.server 8080"
echo "  # then open http://localhost:8080"

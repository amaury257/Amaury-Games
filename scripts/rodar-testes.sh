#!/usr/bin/env bash
# rodar-testes.sh — Compila e roda os testes do motor (C puro) no host.
# Funciona em qualquer Linux/WSL2 com gcc ou clang; não requer Theos.
set -eu
RAIZ="$(cd "$(dirname "$0")/.." && pwd)"
cd "$RAIZ"

CC_HOST="${CC:-cc}"
mkdir -p build
"$CC_HOST" -std=c11 -O1 -Wall -Wextra -o build/testes \
    tests/testes.c Sources/Game/engine/*.c -lm
./build/testes

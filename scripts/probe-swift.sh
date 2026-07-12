#!/usr/bin/env bash
# probe-swift.sh — Teste empírico do D7: a toolchain do Theos consegue
# compilar Swift para arm64-apple-ios a partir do Linux?
# Sai com 0 (sucesso) ou 1 (falha), imprimindo o diagnóstico no stdout.
set -u

if [ -z "${THEOS:-}" ]; then
    echo "THEOS não definido."
    exit 1
fi

# Candidatos a swiftc: toolchain do Theos primeiro, depois o PATH.
SWIFTC=""
for CANDIDATO in \
    "$THEOS/toolchain/linux/iphone/bin/swiftc" \
    "$THEOS/toolchain/swift/bin/swiftc" \
    "$(command -v swiftc || true)"; do
    if [ -n "$CANDIDATO" ] && [ -x "$CANDIDATO" ]; then
        SWIFTC="$CANDIDATO"
        break
    fi
done

if [ -z "$SWIFTC" ]; then
    echo "Nenhum swiftc encontrado (toolchain do Theos ou PATH)."
    exit 1
fi
echo "swiftc: $SWIFTC"

SDK=$(ls -1d "$THEOS"/sdks/iPhoneOS*.sdk 2>/dev/null | sort -V | tail -n1)
if [ -z "$SDK" ]; then
    echo "Nenhum SDK iPhoneOS em \$THEOS/sdks."
    exit 1
fi
echo "SDK: $SDK"

DIR=$(mktemp -d)
trap 'rm -rf "$DIR"' EXIT
cat > "$DIR/hello.swift" <<'FIM'
// Probe mínimo da Fase 0: compilar (não executar) já responde o D7.
import Foundation
print("Olá, River Raid — Swift para iOS via Theos/Linux")
FIM

if "$SWIFTC" -target arm64-apple-ios26.0 -sdk "$SDK" \
        -o "$DIR/hello" "$DIR/hello.swift" 2>&1; then
    echo "Compilação Swift→arm64-apple-ios: OK"
    exit 0
else
    echo "Compilação Swift→arm64-apple-ios: FALHOU"
    exit 1
fi

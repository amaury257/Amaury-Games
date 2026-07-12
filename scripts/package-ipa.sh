#!/usr/bin/env bash
# package-ipa.sh — Monta Payload/RiverRaid.app a partir do staging do Theos
# e gera build/RiverRaid.ipa, pronto para o AltStore reassinar e instalar.
set -eu

RAIZ="$(cd "$(dirname "$0")/.." && pwd)"
cd "$RAIZ"

APP_STAGE=".theos/_/Applications/RiverRaid.app"
if [ ! -d "$APP_STAGE" ]; then
    echo "✗ $APP_STAGE não existe. Rode 'make package FINALPACKAGE=1' antes." >&2
    exit 1
fi

if [ ! -f "$APP_STAGE/Frameworks/libdummy.dylib" ]; then
    echo "⚠ Aviso: Frameworks/libdummy.dylib ausente — o teste de dlopen (D3) vai falhar no aparelho." >&2
fi

rm -rf build/Payload build/RiverRaid.ipa
mkdir -p build/Payload
cp -a "$APP_STAGE" build/Payload/

command -v zip >/dev/null || { echo "✗ 'zip' não instalado (sudo apt install zip)." >&2; exit 1; }
(cd build && zip -qr RiverRaid.ipa Payload)
rm -rf build/Payload

echo "✓ .ipa gerado: build/RiverRaid.ipa ($(du -h build/RiverRaid.ipa | cut -f1))"
echo "  Instale via AltStore: My Apps → + → RiverRaid.ipa (o AltStore reassina app + frameworks)."

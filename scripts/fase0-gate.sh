#!/usr/bin/env bash
# fase0-gate.sh — Gate de toolchain da Fase 0 (rodar no WSL2, na raiz do repo).
#
# Executa, em ordem:
#   1. Verifica $THEOS e lista os SDKs iOS disponíveis.
#   2. Probe Swift-para-iOS (decide D7: Swift vs ObjC).
#   3. Build do app "Olá" (ObjC) com dylib dummy embarcada.
#   4. Empacota o .ipa para instalação via AltStore.
#   5. Grava docs/fase0-resultados.md e imprime o checklist de DoD.
#
# O teste 3 do §11.1 (dlopen após reassinatura) acontece NO APARELHO:
# o próprio app exibe o resultado na tela ao abrir.
set -u

RAIZ="$(cd "$(dirname "$0")/.." && pwd)"
RESULTADOS="$RAIZ/docs/fase0-resultados.md"
cd "$RAIZ"

verde()  { printf '\033[32m%s\033[0m\n' "$*"; }
ambar()  { printf '\033[33m%s\033[0m\n' "$*"; }
erro()   { printf '\033[31m%s\033[0m\n' "$*"; }

echo "== Fase 0 — gate de toolchain =="
echo

# ---------------------------------------------------------------- 1. Theos + SDK
if [ -z "${THEOS:-}" ] || [ ! -d "${THEOS:-/inexistente}" ]; then
    erro "✗ \$THEOS não está definido ou o diretório não existe."
    echo "  Siga docs/build.md (seção 'Instalando o Theos no WSL2') e rode este script de novo."
    exit 1
fi
verde "✓ THEOS = $THEOS"

SDKS=$(ls -1 "$THEOS/sdks" 2>/dev/null | grep -i 'iPhoneOS.*\.sdk' || true)
if [ -z "$SDKS" ]; then
    erro "✗ Nenhum SDK iPhoneOS em \$THEOS/sdks."
    echo "  Baixe um SDK (ver docs/build.md, seção 'SDK iOS') e rode este script de novo."
    exit 1
fi
SDK_MAIS_NOVO=$(echo "$SDKS" | sort -V | tail -n1)
verde "✓ SDKs disponíveis:"
echo "$SDKS" | sed 's/^/    /'
echo "  O Makefile usa 'latest' → $SDK_MAIS_NOVO"

# ---------------------------------------------------------------- 2. Probe Swift (D7)
echo
echo "-- Probe Swift-para-iOS (decide D7) --"
SWIFT_RESULTADO="FALHOU"
SWIFT_DETALHE=""
if SWIFT_DETALHE=$("$RAIZ/scripts/probe-swift.sh" 2>&1); then
    SWIFT_RESULTADO="OK"
    verde "✓ Swift→iOS compilou. D7 pode ficar em Swift 6."
else
    ambar "⚠ Swift→iOS indisponível ou falhou. Conforme D7, o app segue em ObjC/UIKit + C++."
fi
echo "$SWIFT_DETALHE" | sed 's/^/    /'

# ---------------------------------------------------------------- 3. Build do app
echo
echo "-- Build do app Olá (ObjC) + dylib dummy --"
if ! make clean >/dev/null 2>&1; then true; fi
if ! make package FINALPACKAGE=1; then
    erro "✗ Build falhou. Consulte docs/build.md (troubleshooting) e o log acima."
    exit 1
fi
verde "✓ Build ok."

# ---------------------------------------------------------------- 4. .ipa
echo
echo "-- Empacotando .ipa --"
if ! "$RAIZ/scripts/package-ipa.sh"; then
    erro "✗ Empacotamento do .ipa falhou."
    exit 1
fi

# ---------------------------------------------------------------- 5. Resultados
DATA=$(date '+%Y-%m-%d %H:%M')
cat > "$RESULTADOS" <<FIM
# Fase 0 — resultados do gate de toolchain

> Gerado por \`scripts/fase0-gate.sh\` em $DATA (WSL2).

| Verificação | Resultado |
|---|---|
| \$THEOS | \`$THEOS\` |
| SDKs em \$THEOS/sdks | $(echo "$SDKS" | tr '\n' ' ') |
| SDK usado (latest) | $SDK_MAIS_NOVO |
| Probe Swift→iOS (D7) | **$SWIFT_RESULTADO** |
| Build ObjC + dylib dummy | OK |
| .ipa gerado | \`build/RiverRaid.ipa\` |
| dlopen pós-reassinatura (D3) | **pendente — verificar na tela do app no iPhone** |

## Detalhe do probe Swift

\`\`\`
$SWIFT_DETALHE
\`\`\`

## Próximo passo (manual)

1. Instalar \`build/RiverRaid.ipa\` via AltStore no iPhone 14.
2. Abrir o app: a tela mostra o resultado do dlopen da dylib embarcada.
   - **Verde (OK)** ⇒ D3-A confirmado → atualizar \`docs/adr/0001-d3-carregamento-de-cores.md\` para "Aceito".
   - **Âmbar (falha)** ⇒ avaliar D3-B (ver troubleshooting em \`docs/build.md\`).
3. Registrar o resultado do Swift em \`docs/adr/0002-d7-linguagem.md\`.
FIM
verde "✓ Resultados gravados em docs/fase0-resultados.md"

# ---------------------------------------------------------------- Checklist DoD
echo
echo "== Checklist de DoD da Fase 0 =="
echo "  [x] SDK verificado ($SDK_MAIS_NOVO)"
echo "  [x] Teste Swift vs ObjC executado (Swift: $SWIFT_RESULTADO)"
echo "  [x] .ipa gerado: build/RiverRaid.ipa"
echo "  [ ] Instalar via AltStore e abrir no iPhone (smoke test)"
echo "  [ ] Conferir na tela do app o resultado do dlopen (decide D3)"
echo "  [ ] Fechar ADRs 0001 (D3) e 0002 (D7) com os resultados"
echo
echo "Smoke test: o app deve abrir em tela escura com 'RIVER RAID' em verde e"
echo "a mensagem da dylib dummy. Modo avião ligado — nada depende de rede."

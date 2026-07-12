# ADR 0003 — Build primário via GitHub Actions (pipeline do photovault)

- **Status:** Aceito.
- **Data:** 2026-07-12

## Contexto

A spec (§4) fixava "build fecha inteiramente por Theos/WSL2" porque não havia
Mac. Porém o usuário mostrou que seu app **photovault** (mesmo GitHub) já
compila e instala com sucesso há tempos por outro caminho, sem Mac físico e
sem Theos: **GitHub Actions com runner macOS** (Xcode real), archive **sem
assinatura**, `.ipa` publicado numa **source do AltStore** servida por link
direto (`pages/apps.json` via raw.githubusercontent) + Release + artefato.
O AltStore reassina na instalação e mostra atualizações automaticamente.
O usuário pediu explicitamente para usar esse fluxo aqui.

## Decisão

Replicar o pipeline do photovault como **caminho primário de build**:

- `project.yml` (XcodeGen) gera o `RiverRaid.xcodeproj` — app ObjC/C
  (target `RiverRaid`) + framework embarcado `Fase0Dummy` (teste D3).
- `.github/workflows/build-ipa.yml`: job de **testes do motor** no Linux
  (gate de qualidade) → job macOS que arquiva sem assinatura, empacota o
  `.ipa`, publica `pages/apps.json` + `pages/RiverRaid.ipa` no próprio
  branch (`[skip ci]`), atualiza a Release `latest` e sobe artefato.
- Instalação: adicionar a URL do `apps.json` como source no AltStore e
  instalar/atualizar OTA, sem cabo e sem WSL2.

O caminho **Theos/WSL2 permanece como alternativa local** (Makefile e
scripts intactos); o teste de dlopen aceita tanto `libdummy.dylib` (Theos)
quanto `Fase0Dummy.framework` (Xcode).

## Consequências

- Desvio consciente do §4 do prompt mestre, autorizado pelo usuário e
  validado pelo precedente do photovault. As demais restrições seguem:
  offline absoluto em runtime (CI é build-time), zero IP de terceiros.
- O agente de desenvolvimento passa a ter **feedback real de compilação**
  (logs do xcodebuild no CI) — erros de ObjC são corrigidos sem depender
  da máquina do usuário.
- Deployment target ajustado para iOS 16 (o SDK do runner decide o máximo;
  o iPhone 14 do usuário roda iOS atual — sem impacto).
- Cores libretro (Fase 3) serão compilados no runner macOS com o SDK do
  Xcode — mais simples que o plano original com clang do Theos.

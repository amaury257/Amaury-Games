# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

> Idioma de trabalho deste projeto: **pt-BR** em código-comentário, UI, commits e documentação. Este arquivo segue a mesma convenção.

## Estado do repositório

**Fase 1 validada no aparelho real (iPhone 14 do usuário) via AltStore Classic + AltServer.** O build sai do GitHub Actions (ADR 0003), não do Theos/WSL2 — esse caminho local ficou como alternativa.

- **D7 decidido: ObjC/UIKit + C** (ADR 0002) — único caminho garantido sem o probe Swift. Não escrever Swift.
- **D3 CONFIRMADO (ADR 0001, 2026-07-13): dlopen de frameworks embarcados funciona pós-reassinatura da AltStore.** A tela Diagnóstico mostrou verde no aparelho. Fase 3 (cores libretro) está liberada tecnicamente.
- O usuário está na **AltStore Classic** (não PAL — PAL exige Apple ID de região UE, indisponível no Brasil). Instalação/atualização passa por AltServer aberto no Windows + iPhone no mesmo Wi-Fi/cabo; as sources (`pages/apps.json` deste repo e do photovault) foram registradas na aba Sources para evitar `.ipa` manual.
- Jogo testado no aparelho: **ajustes de tuning em andamento** (ver rr_tuning.h) — manche flutuante estava exagerando o movimento (curva de resposta + raio maiores em InputHub.m) e a velocidade base estava rápida demais (RR_VEL_BASE reduzido). Reavaliar após o usuário testar o novo build.

A fonte de verdade completa do projeto é **`docs/prompt-mestre.md`** (Prompt Mestre v2.0) — leia-o antes de qualquer decisão estrutural. Este CLAUDE.md destila o essencial; em caso de dúvida ou conflito, o prompt mestre prevalece (exceto onde um ADR registrou desvio consciente).

Respostas do §15 (já coletadas do usuário — não perguntar de novo):
1. SDK iOS: desconhecido — o gate detecta (`Makefile` usa `latest`); resultado vai para `docs/fase0-resultados.md`.
2. Toolchain Swift: irrelevante agora — D7 fixado em ObjC (ADR 0002).
3. iPhone: **iPhone 14** (tela 60 Hz, sem ProMotion; Core Haptics disponível).
4. Manche virtual: **flutuante** (aparece onde tocar) — implementado no InputHub.

Campos preenchidos: bundle ID `com.amaury.riverraid`; nome de exibição "River Raid".

## Visão do produto

Um único app iOS, **offline absoluto**, para uso pessoal sideloaded (nunca distribuído), com dois pilares sobre a mesma infraestrutura:

1. **"River Raid"** — recriação fiel das mecânicas do clássico de 1982, com assets 100% próprios, retrato, 160×192 @ 60 fps. É a Fase 1 e prova o pipeline.
2. **Emulador multi-sistema** — frontend premium para cores libretro: Stella (Atari 2600) e Snes9x (SNES), com importação de ROMs pelo usuário, save states, rewind, fast-forward e filtro CRT.

## Restrições inegociáveis (§0/§4 do prompt mestre)

Em qualquer conflito entre uma escolha técnica e estas regras, **pare, exponha o conflito e aguarde o usuário** — nunca relaxe silenciosamente:

- **OFFLINE ABSOLUTO.** Zero chamadas de rede em runtime: sem analytics, telemetria, CDN ou scraping de capas. Critério de aceite: o app funciona por completo em modo avião.
- **Sem IP de terceiros no bundle**: nenhuma ROM, BIOS, sprite, som, fonte ou arte extraída de jogo comercial. Recriam-se mecânicas (não protegidas) com assets redesenhados. ROMs são importadas pelo usuário, fora do bundle.
- **Build fecha inteiramente por Theos/WSL2.** Não há Mac nem Xcode — qualquer passo que exigir Xcode deve ser marcado `⚠️ REQUER MAC` com alternativa Theos. Dependências C/C++ como submódulos git em `external/`, compiladas localmente; preferir submódulos a CocoaPods/SPM remoto.
- **Alvo: iPhone físico, iOS 26.5+**, APIs modernas, sem código legado condicional. Assinatura/instalação via AltStore (Apple ID gratuito: 3 apps ativos, revalidação a cada 7 dias).
- **pt-BR** em UI, comentários, commits e docs.

## Decisões de engenharia já tomadas (D1–D8)

Não reabrir sem novo ADR; justificativas completas no §5 do prompt mestre:

- **D1** — Integrar cores libretro prontos (Stella, Snes9x); nunca escrever emulador do zero.
- **D2** — JIT desnecessário para 2600/SNES. Apenas prever campo `requiresJIT: Bool` no descritor de core para sistemas futuros.
- **D3** — Cores como **frameworks/dylibs embarcados** em `.app/Frameworks/`, carregados via `dlopen` (Opção A). Motivo crítico: linkagem estática de dois cores colide símbolos (`retro_run` etc.). Fallback B (estático + renomeação de símbolos) só se a reassinatura do AltStore falhar — validar na Fase 0.
- **D4** — Vídeo: **Metal** (CAMetalLayer + CADisplayLink), textura por frame via `replaceRegion`, nearest sampling, escala inteira.
- **D5** — Áudio: **`AVAudioSourceNode`** (pull) + ring buffer + controle dinâmico de taxa (±0,5% conforme preenchimento, alvo ~64 ms) para eliminar estalos.
- **D6** — Mapa do River Raid: **LFSR Fibonacci de 16 bits**, taps [16, 14, 13, 11], seed `0xACE1` no modo Clássico. Mesmo seed ⇒ mesmo mundo; testável por golden test.
- **D7** — **DECIDIDO: ObjC/UIKit + C** (ADR 0002). O motor do jogo é C puro em `Sources/Game/engine/` (portável, determinístico, testável no host); a UI e a infra são ObjC/ARC. Não escrever Swift.
- **D8** — Ponte com cores em Objective-C++ (`LibretroCore.mm`).

## Arquitetura-alvo

O coração é o protocolo **`GameSession`** (assinaturas de referência no §6 do prompt mestre): tanto o jogo River Raid quanto os cores libretro o implementam — a UI não sabe a diferença. Infraestrutura compartilhada em `Core/`: `VideoRenderer` (Metal), `AudioEngine` (AVAudioSourceNode + ring buffer + DRC), `InputHub` (touch overlay + GameController), `StateStore` (saves, SRAM, recordes), `Log` (JSON estruturado local, nunca rede).

**Threading:** thread de emulação dedicada (QoS `.userInteractive`) cadenciada por `CADisplayLink` fixado em 60 Hz (`preferredFrameRateRange`; em ProMotion 120 Hz, apresentar cada frame 2×). Input lido imediatamente antes de cada `runFrame`.

Estrutura de pastas planejada (§10):

```
RiverRaid/
├── Makefile                  # Theos, template application
├── control
├── Resources/                # ícones, fontes próprias, LaunchScreen
├── RiverRaid.plist           # Info.plist (chaves obrigatórias no §11.4)
├── Sources/
│   ├── App/                  # entry, roteador, injeção
│   ├── Shell/                # Home, Importador, Configurações, QuickMenu
│   ├── Core/                 # GameSession, VideoRenderer, AudioEngine, InputHub, StateStore, Log
│   ├── Game/                 # River Raid: GameTuning, LFSRWorld, entidades, colisão, HUD, synth
│   └── Emu/Bridge/           # LibretroCore.mm, libretro.h
├── external/                 # submódulos: stella-libretro, snes9x
├── scripts/                  # build.sh, package-ipa.sh, gerar-icones.sh, build-cores.sh
└── docs/                     # build.md, cores.md, arquitetura.md, controles.md, adr/, CHANGELOG.md
```

Constantes de gameplay centralizadas em `Sources/Game/engine/rr_tuning.h` (defaults tunáveis; o §10 citava `GameTuning.swift`, ajustado pelo D7). Detalhes de implementação da Fase 1 em `docs/arquitetura.md`. Layout de dados do emulador: `Documents/roms/<sistema>/` e `Documents/saves/<sistema>/<sha1-da-rom>/` — identidade de ROM por SHA-1.

## Build e deploy

**Pipeline primário (ADR 0003, herdado do photovault do usuário):** GitHub Actions → runner macOS → XcodeGen (`project.yml`) → `xcodebuild` sem assinatura → `.ipa` → source do AltStore (`pages/apps.json`, link raw) + Release `latest` + artefato. Workflow: `.github/workflows/build-ipa.yml`, dispara em push (`main` e `claude/**`); o job de build só roda se os testes do motor passarem no Linux. O CI commita `pages/` no próprio branch com `[skip ci]` — **rode `git pull --rebase` antes de push**. Monitorar/corrigir builds pelo próprio CI (logs do xcodebuild) — é o único lugar onde o ObjC compila de verdade.

**Caminho alternativo local:** Theos/WSL2 (Makefile; docs/build.md). O teste de dlopen do Diagnóstico aceita `Fase0Dummy.framework` (Xcode) ou `libdummy.dylib` (Theos).

- **Testes do motor (rodam em qualquer host, sem Theos): `./scripts/rodar-testes.sh`** — golden LFSR/mapa, combustível, colisão, vida extra, save state, recordes. Rodar sempre que tocar em `Sources/Game/engine/`.
- Gate completo da Fase 0: `./scripts/fase0-gate.sh` (verifica Theos/SDK, probe Swift, build, `.ipa`, grava `docs/fase0-resultados.md`).
- Build manual: `make package FINALPACKAGE=1` → `./scripts/package-ipa.sh` gera `build/RiverRaid.ipa`.
- Makefile: template **application** do Theos; `TARGET := iphone:clang:latest:26.0`; `ARCHS := arm64`. O Info.plist vive em `Resources/Info.plist` (convenção do Theos; o §10 citava `RiverRaid.plist` na raiz). Frameworks adicionais (Metal, MetalKit, AVFoundation, GameController, CoreHaptics) entram nas fases seguintes.
- `scripts/build-cores.sh` (Fase 3) compilará os submódulos libretro antes do app (clang+SDK do Theos, `-miphoneos-version-min=26.0`, arm64); flags exatas irão para `docs/cores.md`.
- Instalação: AltStore reassina o `.ipa` (app + frameworks embarcados) com o Apple ID do usuário. Entitlements mínimos.
- Instalação do Theos/SDK e troubleshooting: `docs/build.md`.

## Fases e Definition of Done (§13)

Executar em ordem; **nunca deixar o build quebrado entre fases** — cada fase termina com `.ipa` instalável, smoke test manual descrito, CHANGELOG atualizado e checklist de DoD reportado item a item:

| Fase | Entrega | DoD |
|---|---|---|
| **0** | Gate de toolchain (§11.1): SDK verificado, teste Swift vs ObjC, teste dylib+dlopen reassinado pelo AltStore | `.ipa` "Olá" instalado; D3 e D7 decididos em ADR |
| **1** | River Raid completo (§7) | 60 fps; Clássico + Diário; recordes; touch + gamepad; testes §7.13 verdes; modo avião ok |
| **2** | Infra `Core/` consolidada | Jogo refatorado sobre `GameSession`/VideoRenderer/AudioEngine/InputHub sem regressão |
| **3** | Stella (2600) ponta a ponta | ROM homebrew livre roda com vídeo+áudio+input+chaves do console+save state+rewind |
| **4** | Snes9x (SNES) | Idem + SRAM persistente + overlay SNES + paisagem |
| **5** | Polimento premium | CRT shader, fast-forward, thumbnails, ícone final, acessibilidade, docs completos |

## Convenções

- **Commits pt-BR** no padrão `feat:/fix:/docs:/refactor:`.
- **Planejar antes de codar**: produza o plano da fase corrente, execute, só então avance.
- Manter **`docs/CHANGELOG.md`** (pt-BR) e **ADRs curtos em `docs/adr/`** para toda decisão estrutural.
- ObjC disciplinado (D7/ADR 0002): ARC em tudo, nil-checks em fluxo crítico, erros via `NSError`; motor em C99 sem alocação dinâmica no caminho do frame.
- Log JSON estruturado em arquivo rotacionado no sandbox (`{ts, nivel, modulo, evento, dados}`), nunca rede.
- **UI "CRT-noir" premium obrigatória em todas as telas** (§9): tipografia própria, nada de SF System puro, nada com "cara de protótipo". Acessibilidade: contraste AA, alvos ≥ 44 pt, Dynamic Type.
- Detalhes de gameplay (pontuação canônica, combustível, pontes-checkpoint, vidas, áudio TIA sintetizado) estão no §7 do prompt mestre — segui-lo à risca no modo Clássico; extras modernos apenas como toggles desligados por padrão.

## Testes obrigatórios

Harness: `tests/testes.c` compilado pelo host (`./scripts/rodar-testes.sh`) — o motor é C puro, então os testes do jogo não dependem de Theos nem de aparelho. Cobertos hoje (§7.13 e §12):

- **Golden test do LFSR/mapa**: mesma seed ⇒ sequência de peças byte-idêntica.
- Colisão AABB (casos de borda em margens e ilhas).
- Matemática de combustível (queima/recarga determinísticas).
- Round-trip de recorde e de save state (serialize → unserialize → hash do framebuffer idêntico).
- Pontuação/vida extra em 10.000 pontos.
- Detecção de sistema por header de ROM.

Orçamento de performance: 60 fps sustentados, memória estável, áudio sem estalos por 10 min contínuos (teste manual roteirizado).

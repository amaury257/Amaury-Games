# CHANGELOG

Todas as mudanças relevantes do projeto, em pt-BR. Formato livre inspirado no
[Keep a Changelog](https://keepachangelog.com/pt-BR/); fases conforme §13 do
`docs/prompt-mestre.md`.

## [Não lançado] — Primeiro teste no aparelho + ajustes de tuning

### Confirmado
- **D3 (ADR 0001) aceito**: dlopen de frameworks embarcados sobrevive à
  reassinatura da AltStore Classic — testado no iPhone 14 do usuário,
  Diagnóstico mostrou verde. Fase 3 (cores libretro) liberada.
- Pipeline GitHub Actions → AltStore validado ponta a ponta: build, source
  (`pages/apps.json`), instalação via AltServer (Classic, região BR — PAL
  indisponível fora da UE).

### Ajustado (relato do usuário no aparelho)
- `Sources/Core/InputHub.m`: manche flutuante exagerava o movimento —
  aumentado o raio de saturação (44pt → 60pt) e adicionada curva de resposta
  suave perto do centro (`sign(v)·v²`), cheia na borda. Não introduz inércia
  (§7.3 continua respeitado: resposta no mesmo quadro).
- `Sources/Game/engine/rr_tuning.h`: velocidade rápida demais mesmo em
  cruzeiro — `RR_VEL_BASE` 1.25 → 0.95, `RR_VEL_LATERAL` 1.6 → 1.3.

### Pendente
- Novo teste no aparelho para validar os ajustes de tuning acima.

## [Não lançado] — Pipeline de build via GitHub Actions (ADR 0003)

### Adicionado
- Pipeline do photovault replicado: `project.yml` (XcodeGen, app RiverRaid +
  framework Fase0Dummy embarcado) e `.github/workflows/build-ipa.yml`
  (testes no Linux → archive sem assinatura no macOS → `.ipa` → source do
  AltStore em `pages/apps.json` + Release `latest` + artefato).
- Ícone próprio gerado por script (`scripts/gerar-icone.py` → `assets/icone.png`
  e `Resources/Assets.xcassets`), arte pixel original (jato sobre o rio).
- Diagnóstico (dlopen/D3) aceita `Fase0Dummy.framework` (Xcode) ou
  `libdummy.dylib` (Theos).

### Mudado
- **WSL2/Theos deixa de ser pré-requisito** — vira caminho alternativo local.
  Instalação passa a ser OTA pela source do AltStore (`docs/proximos-passos.md`
  reescrito). Deployment target do build Xcode: iOS 16.

## [Não lançado] — Fase 1 implementada (validação no aparelho pendente)

> Estratégia acordada com o usuário: desenvolver adiantado no ambiente remoto
> e validar depois no WSL2 + iPhone (o gate da Fase 0 ainda não rodou).
> Por isso **D7 foi fixado em ObjC/UIKit + C** (ADR 0002) — o único caminho
> garantido no Theos sem o probe Swift.

### Adicionado — motor do jogo (C puro, `Sources/Game/engine/`)
- `rr_lfsr.h`: LFSR Fibonacci 16 bits, taps [16,14,13,11] (D6), golden test.
- `rr_mundo.c`: geração determinística de seções (8 tipos de peça, ponte ao
  final, dificuldade progressiva, entidades por proximidade via LFSR).
- `rr_jogo.c`: acelerador no manche (0,5×–2,0×), lateral sem inércia, 2 mísseis
  que herdam velocidade, combustível (60 s, recarga ~4 s, alarme < 25%),
  pontuação canônica (30/60/80/100/500), vidas + extra a cada 10.000,
  pontes-checkpoint com respawn invulnerável (~1,5 s), colisão AABB,
  save state versionado (POD + memcpy).
- `rr_render.c`: renderizador por software 160×192 XRGB8888, paleta ~10 cores,
  sprites próprios, explosão procedural de 4 quadros, HUD com medidor
  analógico E·½·F, fonte 3×5 própria.
- `rr_audio.c`: sintetizador estilo TIA 2 canais (drone de motor acoplado ao
  acelerador, blip de tiro, tom de recarga, jingle de vida extra, ruído de
  explosão, alarme de combustível).
- `rr_recordes.c`: top 10 com nome de 3 letras, serialização round-trip.

### Adicionado — infraestrutura (`Sources/Core/`)
- `GameSession.h`: protocolo unificado do §6 (o jogo e os futuros cores).
- `VideoRenderer`: Metal com shader compilado em runtime, nearest, escala inteira.
- `AudioEngine`: AVAudioSourceNode em pull, sessão `.ambient`.
- `InputHub`: manche flutuante + botão de fogo + GCController mesclado.
- `EmuLoop`: thread dedicada QoS userInteractive, CADisplayLink fixo 60 Hz.
- `StateStore`: recordes por modo em `Documents/recordes/` (Diário por dia).
- `Log`: JSON estruturado rotacionado em `Documents/logs/app.log`.

### Adicionado — Shell (`Sources/Shell/`)
- Home CRT-noir (Clássico, Diário, Recordes, Diagnóstico), efeito power-on de
  CRT na entrada da sessão, pausa com quick menu, fim de jogo com teclado
  retrô de 3 letras, tela de recordes Clássico/Diário.

### Testes (`tests/testes.c`, rodam em qualquer host)
- 34 verificações: golden do LFSR e do mapa, determinismo de framebuffer,
  matemática de combustível, colisão AABB (casos de borda), vida extra em
  10.000, round-trip de save state (hash de framebuffer) e de recordes,
  seed do Diário. Soak de 10 min simulados com ASan/UBSan limpo (85 seções).

### Corrigido (revisão completa de código)
- Mundo: ilha logo após trecho estreito podia deixar passagem menor que o
  jato (morte inevitável em seções difíceis) — o limite da ilha agora usa a
  menor largura do canal ao longo de toda a peça (passagem mínima de 14 px).
- Makefile: a regra da dylib dummy dependia de variáveis internas do Theos
  (`TARGET_CC`/`SYSROOT`); agora detecta clang e SDK explicitamente.
- AudioEngine: `dealloc` liberava o buffer sem parar o engine antes
  (use-after-free potencial no callback de áudio).
- JogoViewController: sair da tela durante a animação de power-on iniciava o
  loop de emulação órfão (vazamento de thread permanente).
- Revisão validada: 34 testes verdes, soak de 10 min com ASan/UBSan limpo,
  passe estrito `-Wconversion` sem avisos no motor.

### Pendente
- Rodar `scripts/fase0-gate.sh` no WSL2 (D3/dlopen ainda sem veredito) e
  smoke test da Fase 1 no iPhone: 60 fps, áudio sem estalos, modo avião.
  **Roteiro completo em `docs/proximos-passos.md`.**
- Tutorial de 1 tela, Configurações, tilt opcional e tipografia própria.

## Fase 0 — scaffold

### Adicionado
- Scaffold do projeto Theos (template application): `Makefile` (SDK auto-detectado
  via `latest`, alvo iOS 26.0, arm64), `control`, `Resources/Info.plist` com as
  chaves do §11.4.
- App "Olá" da Fase 0 em ObjC/UIKit (`Sources/App/`): tela CRT-noir mínima que
  executa e exibe o teste de dlopen da dylib dummy embarcada (decide D3).
- Dylib dummy (`Sources/Fase0/dummy.c`) embarcada em `Frameworks/` no staging.
- Scripts do gate: `scripts/fase0-gate.sh` (orquestra o gate e grava
  `docs/fase0-resultados.md`), `scripts/probe-swift.sh` (decide D7),
  `scripts/package-ipa.sh` (gera `build/RiverRaid.ipa`).
- Documentação: `docs/build.md` (instalação Theos/SDK, gate, AltStore,
  troubleshooting), ADRs 0001 (D3) e 0002 (D7) em estado "Proposto".
- `CLAUDE.md` e `docs/prompt-mestre.md` (Prompt Mestre v2.0).

### Decidido (respostas do §15)
- Bundle ID `com.amaury.riverraid`; nome de exibição "River Raid".
- Aparelho alvo: iPhone 14 (60 Hz, sem ProMotion; Core Haptics disponível).
- Manche virtual **flutuante** (aparece onde tocar) como padrão da Fase 1.

### Pendente para fechar a Fase 0
- Rodar `scripts/fase0-gate.sh` no WSL2, instalar o `.ipa` via AltStore e
  reportar o resultado do dlopen (fecha ADRs 0001/0002).

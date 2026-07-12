# CHANGELOG

Todas as mudanças relevantes do projeto, em pt-BR. Formato livre inspirado no
[Keep a Changelog](https://keepachangelog.com/pt-BR/); fases conforme §13 do
`docs/prompt-mestre.md`.

## [Não lançado] — Fase 0 em andamento

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

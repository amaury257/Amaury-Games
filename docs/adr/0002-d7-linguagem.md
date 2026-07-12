# ADR 0002 — D7: Linguagem principal do app

- **Status:** **Aceito — Objective-C/UIKit + C.**
- **Data da decisão:** 2026-07-12

## Contexto

Preferência original da spec: Swift 6, com decisão empírica na Fase 0 (probe
de compilação Swift→iOS pelo Theos/Linux). O usuário, porém, optou por
**desenvolver o app inteiro antes de rodar o gate** no WSL2 dele, validando
depois no aparelho. Sem o resultado do probe, escrever o app em Swift seria
apostar todo o código numa toolchain não verificada.

## Decisão

**D7 = Objective-C/UIKit para todo o app**, com o motor do jogo em **C puro**
(`Sources/Game/engine/`) — portável, determinístico e testável em qualquer
host (os testes do §7.13 rodam no Linux via `scripts/rodar-testes.sh`).
A ponte com os cores libretro segue em Objective-C++ (D8), como planejado.

É o único caminho que garante build no Theos/Linux independentemente do
resultado do probe Swift (`scripts/probe-swift.sh`, que permanece no gate a
título informativo). A spec sempre tratou ObjC como plenamente viável.

## Consequências

- Zero risco de toolchain: clang do Theos compila ObjC e C nativamente.
- Disciplina exigida no ObjC: ARC em todo o app, sem force-unwrap equivalente
  (nil-checks em fluxo crítico), erros via NSError.
- O motor em C dá de graça o determinismo exigido pelo §7.2 e os golden tests.
- Migração futura a Swift é possível módulo a módulo (a fronteira é o
  protocolo `GameSession`), mas não está planejada.

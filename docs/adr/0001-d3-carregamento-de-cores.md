# ADR 0001 — D3: Carregamento dos cores libretro

- **Status:** Proposto — aguardando resultado do gate da Fase 0 no aparelho.
- **Data:** 2026-07-12

## Contexto

Dois cores libretro (Stella, Snes9x) exportam os mesmos símbolos C
(`retro_run`, `retro_load_game`, …); linkados estaticamente no mesmo binário,
os símbolos colidem. O fluxo padrão de frontends libretro é carregar cada core
como dylib via `dlopen`. A incógnita: o AltStore precisa reassinar os
frameworks embarcados em `.app/Frameworks/` para o `dlopen` funcionar num app
sideloaded com Apple ID gratuito.

## Decisão (proposta)

**Opção A** — cada core compilado como dylib/framework embarcado em
`.app/Frameworks/`, carregado via `dlopen`.

**Fallback B** (somente se A falhar na reassinatura): linkagem estática com
renomeação de símbolos por core (prefixos `stella_`/`snes9x_` aplicados por
script no build).

## Critério de decisão

O app da Fase 0 embarca `Frameworks/libdummy.dylib` e exibe na tela o
resultado do `dlopen` + chamada de função após instalação via AltStore:

- **Verde (OK)** ⇒ confirmar A; status deste ADR vira **Aceito**.
- **Âmbar (falha de assinatura persistente)** ⇒ adotar B; registrar novo ADR
  com o mecanismo de renomeação.

## Consequências

- A: pipeline de build dos cores gera dylibs (`scripts/build-cores.sh`,
  Fase 3); `LibretroCore.mm` resolve símbolos via `dlsym`.
- B: build fica mais frágil (etapa extra de objcopy/renome) e `LibretroCore.mm`
  chama funções prefixadas por tabela.

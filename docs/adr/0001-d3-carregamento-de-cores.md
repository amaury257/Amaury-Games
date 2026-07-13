# ADR 0001 — D3: Carregamento dos cores libretro

- **Status:** **Aceito — Opção A confirmada no aparelho.**
- **Data da decisão:** 2026-07-13
- **Data de abertura:** 2026-07-12

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

O app da Fase 0 embarca a dylib/framework dummy e exibe na tela o resultado
do `dlopen` + chamada de função após instalação via AltStore:

- **Verde (OK)** ⇒ confirmar A; status deste ADR vira **Aceito**.
- **Âmbar (falha de assinatura persistente)** ⇒ adotar B; registrar novo ADR
  com o mecanismo de renomeação.

## Resultado

Testado no iPhone 14 do usuário (build via GitHub Actions, ADR 0003;
`Fase0Dummy.framework` embarcado, reassinado e instalado pela AltStore
Classic + AltServer). Tela Diagnóstico mostrou:

> ✓ Reassinatura AltStore + dlopen: OK — D3-A (frameworks embarcados) confirmado

**Opção A adotada.** Frameworks/dylibs embarcados em `.app/Frameworks/`,
carregados via `dlopen`, sobrevivem à reassinatura da AltStore Classic sem
ressalvas. Libera a Fase 3 (cores libretro) para seguir por esse caminho.

## Consequências

- Pipeline de build dos cores (Fase 3) compila Stella/Snes9x como
  frameworks/dylibs no runner macOS do CI (mesmo mecanismo do `project.yml`
  atual) e os embarca em `Frameworks/`.
- `LibretroCore.mm` resolve os símbolos de cada core via `dlopen`/`dlsym`,
  sem colisão entre `retro_run` de cores diferentes (motivo original da D3).
- Fallback B (estático + renomeação de símbolos) descartado — não será
  necessário a menos que uma versão futura da AltStore quebre esse fluxo.

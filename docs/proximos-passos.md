# Próximos passos — guia passo-a-passo

> **Atualizado (ADR 0003):** o projeto agora usa o MESMO pipeline do seu
> photovault — GitHub Actions compila o `.ipa` e publica uma source do
> AltStore. **Você não precisa mais de WSL2 nem de Theos.**

---

## Passo 1 — Esperar o build verde no GitHub (automático)

A cada push meu, o workflow **"Build IPA (não assinado)"** roda sozinho:
primeiro os testes do motor, depois o build no runner macOS.

Acompanhe em: `https://github.com/amaury257/Amaury-Games/actions`

Eu mesmo monitoro esses builds e corrijo falhas — quando estiver verde,
o `.ipa` novo já está publicado automaticamente.

## Passo 2 — Adicionar a source no AltStore (uma vez só)

No iPhone, abra o AltStore → aba **Sources** → **+** e cole:

```
https://raw.githubusercontent.com/amaury257/Amaury-Games/claude/claude-md-docs-1fubba/pages/apps.json
```

> É o mesmo esquema do seu iAmaury/photovault: link direto via
> raw.githubusercontent, sem redirect. Se um dia o projeto for para o
> branch `main`, a URL muda de acordo (te aviso).

## Passo 3 — Instalar o River Raid

Na source "Amaury Games", toque em **River Raid → Instalar**.
O AltStore reassina com seu Apple ID (mesmos limites de sempre: 3 apps,
renovação a cada 7 dias). Atualizações futuras aparecem no próprio AltStore.

Alternativas de download direto do `.ipa`, se preferir:
- Release "latest": `https://github.com/amaury257/Amaury-Games/releases`
- Artefato do workflow na aba Actions.

## Passo 4 — Roteiro de teste no iPhone (me reporte item a item)

Ligue o **modo avião** antes (o app deve funcionar 100% offline).

| # | O que fazer | O que observar |
|---|---|---|
| 1 | Abrir o app | Home escura com "RIVER RAID" em verde |
| 2 | Tocar em **DIAGNÓSTICO** | **VERDE** ("dylib carregada") ou **ÂMBAR** (erro)? ← decide o rumo do emulador (D3) |
| 3 | **JOGAR · CLÁSSICO** | Efeito de CRT ligando; rio azul, margens verdes, jato amarelo |
| 4 | Tocar e arrastar na metade esquerda | Manche aparece onde tocou; jato responde sem atraso |
| 5 | Manche para **cima/baixo** | Cenário acelera/freia; ronco do motor muda de tom |
| 6 | Segurar a metade direita | Tiros em cadência; navios (30), helicópteros (60) explodem |
| 7 | Sobrevoar um depósito **GAS** | Combustível sobe; tom ascendente contínuo |
| 8 | Combustível abaixo de ¼ | Agulha pisca; bipe de alarme periódico |
| 9 | Atirar na **ponte** no fim da seção | +500; seção sobe; morrer depois → renasce na ponte |
| 10 | Perder as 3 vidas | "FIM DE JOGO" e, se pontuou, teclado retrô de 3 letras |
| 11 | Home → **RECORDES** | Nome e pontos gravados |
| 12 | **DESAFIO DIÁRIO** | Rio diferente do Clássico (muda a cada dia) |
| 13 | Jogar ~10 min | 60 fps constantes, áudio sem estalos |
| 14 | Gamepad Bluetooth (se tiver) | Analógico move, A atira, Menu pausa |

## Passo 5 — Me reportar

1. **Diagnóstico:** verde ou âmbar (e a mensagem, se âmbar)?
2. **Checklist:** o que passou / falhou / ficou estranho.
3. Ajustes de *game feel* (velocidades, dificuldade, cadência — tudo em
   `Sources/Game/engine/rr_tuning.h`).

## Passo 6 — O que vem depois (eu faço, na ordem)

| Etapa | Conteúdo | Depende de |
|---|---|---|
| Fechar Fase 1 | Correções do seu reporte; tutorial de 1 tela; Configurações | Seu reporte |
| Fase 2 | Consolidar `Core/` (já nasceu sobre `GameSession`; curta) | Fase 1 ok |
| **Fase 3** | **Emulador Atari 2600** (core Stella compilado no CI macOS, importador de ROMs, chaves do console, save states, rewind) | **Diagnóstico VERDE** |
| Fase 4 | Emulador SNES (Snes9x) + SRAM + paisagem | Fase 3 |
| Fase 5 | Polimento: shader CRT, fast-forward, thumbnails, tipografia própria | Fase 4 |

> Caminho antigo (Theos/WSL2) continua documentado em `docs/build.md` como
> alternativa local — mas o fluxo acima dispensa tudo isso.

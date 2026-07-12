# Próximos passos — guia passo-a-passo

> Estado atual: **Fase 1 (River Raid) implementada e revisada**, com testes do
> motor verdes. O que falta é **na sua máquina**: compilar, instalar no iPhone
> e me reportar o resultado. Siga na ordem.

---

## Passo 1 — Obter o código no WSL2

Abra o Ubuntu (WSL2) e rode:

```bash
git clone https://github.com/amaury257/Amaury-Games.git
cd Amaury-Games
git checkout claude/claude-md-docs-1fubba
```

Se já tiver clonado antes, só atualize:

```bash
cd Amaury-Games
git checkout claude/claude-md-docs-1fubba
git pull
```

## Passo 2 — (Opcional, 10 segundos) Confirmar o motor do jogo

Não precisa de Theos — compila com o gcc do próprio Ubuntu:

```bash
sudo apt install -y build-essential   # se ainda não tiver
./scripts/rodar-testes.sh
```

**Esperado:** lista de `ok` terminando em `Todos os testes passaram.`
Se falhar algo aqui, pare e me mande a saída.

## Passo 3 — Instalar o Theos + SDK iOS (só na primeira vez)

Se você ainda não tem o Theos no WSL2, siga **`docs/build.md`** (seções
"Instalando o Theos no WSL2" e "SDK iOS"). Resumo:

```bash
sudo apt install -y build-essential fakeroot git perl zip unzip curl
bash -c "$(curl -fsSL https://raw.githubusercontent.com/theos/theos/master/bin/install-theos)"
# garanta no ~/.bashrc:  export THEOS=~/theos
# e instale um SDK em $THEOS/sdks (comandos exatos no build.md)
```

Verificação rápida: `ls $THEOS/sdks` deve listar algo como `iPhoneOSxx.x.sdk`.

## Passo 4 — Compilar e gerar o .ipa

```bash
./scripts/fase0-gate.sh
```

O script verifica o Theos/SDK, testa a toolchain Swift (só informativo — o
app é ObjC), compila tudo e gera **`build/RiverRaid.ipa`**, gravando um
relatório em `docs/fase0-resultados.md`.

**Se o build falhar:** copie TODO o log de erro do compilador e me mande.
É o cenário esperado na primeira tentativa — eu desenvolvi sem o SDK do iOS
disponível aqui, então pequenos ajustes de compilação são normais e rápidos
de corrigir. Consulte também a tabela de troubleshooting em `docs/build.md`.

## Passo 5 — Instalar no iPhone via AltStore

1. Copie `build/RiverRaid.ipa` para onde o AltStore alcance
   (ex.: `cp build/RiverRaid.ipa /mnt/c/Users/SEU_USUARIO/Desktop/`).
2. Com o AltServer rodando no Windows e o iPhone na mesma rede/cabo:
   AltStore no iPhone → **My Apps → + → RiverRaid.ipa**.
3. Lembretes da conta gratuita: máx. **3 apps ativos**; o app expira em
   **7 dias** (renove pelo AltStore).

## Passo 6 — Roteiro de teste no iPhone (me reporte item a item)

Ligue o **modo avião** antes de começar (o app deve funcionar 100% offline).

| # | O que fazer | O que observar |
|---|---|---|
| 1 | Abrir o app | Home escura com "RIVER RAID" em verde |
| 2 | Tocar em **DIAGNÓSTICO** | **VERDE** ("dylib carregada") ou **ÂMBAR** (erro)? ← decide o rumo do emulador (D3) |
| 3 | **JOGAR · CLÁSSICO** | Efeito de CRT ligando; rio azul, margens verdes, jato amarelo |
| 4 | Tocar e arrastar na metade esquerda | Manche aparece onde tocou; jato move sem atraso |
| 5 | Arrastar o manche para **cima/baixo** | Cenário acelera/freia; ronco do motor muda de tom |
| 6 | Segurar a metade direita | Tiros em cadência; navios (30), helicópteros (60) explodem somando pontos |
| 7 | Sobrevoar um depósito **GAS** | Agulha do combustível sobe; tom ascendente contínuo |
| 8 | Deixar o combustível baixar de ¼ | Agulha pisca; bipe de alarme periódico |
| 9 | Atirar na **ponte** no fim da seção | +500; número da seção sobe; morrer depois → renasce na ponte |
| 10 | Perder as 3 vidas | "FIM DE JOGO" e, se pontuou, teclado retrô de 3 letras |
| 11 | Voltar à Home → **RECORDES** | Seu nome/pontos gravados |
| 12 | **DESAFIO DIÁRIO** | Rio diferente do Clássico (muda a cada dia) |
| 13 | Jogar ~10 min seguidos | Fluidez constante (60 fps), áudio sem estalos, sem aquecer demais |
| 14 | Gamepad Bluetooth (se tiver) | Analógico move, A atira, Menu pausa |

## Passo 7 — Me reportar

Volte aqui e me diga:

1. **Build:** compilou de primeira? Se não, o log de erro completo.
2. **Diagnóstico:** verde ou âmbar (e a mensagem, se âmbar).
3. **Checklist do Passo 6:** o que passou e o que falhou/estranhou
   (ex.: "manche ok mas o jato anda rápido demais", "áudio estala ao atirar").
4. Ajustes de *game feel* que quiser (velocidades, cadência, dificuldade —
   tudo é tunável em `Sources/Game/engine/rr_tuning.h`).

## Passo 8 — O que vem depois (eu faço, na ordem)

| Etapa | Conteúdo | Depende de |
|---|---|---|
| Fechar Fase 1 | Corrigir o que o Passo 7 apontar; tutorial de 1 tela; Configurações | Seu reporte |
| Fase 2 | Consolidar `Core/` (já nasceu sobre `GameSession`, será curta) | Fase 1 ok |
| **Fase 3** | **Emulador Atari 2600** (core Stella via submódulo, `LibretroCore.mm`, importador de ROMs, chaves do console, save states, rewind) | **Diagnóstico VERDE** (D3-A). Se âmbar: fallback D3-B (mais trabalho) |
| Fase 4 | Emulador SNES (Snes9x) + SRAM + paisagem | Fase 3 |
| Fase 5 | Polimento: shader CRT, fast-forward, thumbnails, ícone, tipografia própria | Fase 4 |

> Regra de ouro do projeto: entre uma fase e outra o app **sempre** continua
> instalável e funcional. Qualquer conflito com as restrições (offline
> absoluto, zero IP de terceiros) é trazido a você antes de qualquer código.

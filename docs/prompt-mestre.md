# PROMPT MESTRE — "River Raid" + Emulador Retro Multi-Sistema para iOS

> **Versão 2.0 — revisão completa.** Cole este bloco inteiro na IA de desenvolvimento (Claude Code / arquitetura multi-agente).
> Idioma de trabalho: **pt-BR** em código-comentário, UI, commits e documentação.
> Preencha os campos `<< >>` antes de rodar (lista no §15).

---

## 0. Nota de propriedade intelectual (leia uma vez, aplique sempre)

- O nome **"River Raid"** é usado como título do projeto por decisão do usuário, para **app pessoal sideloaded no próprio aparelho**. Se este app for algum dia distribuído a terceiros (mesmo gratuitamente), o nome deve ser trocado — é marca da Activision.
- **Proibido no bundle:** qualquer ROM, BIOS, sprite, som, fonte ou arte extraída de jogo comercial. A recriação é de **mecânicas** (não protegidas) com **assets 100% redesenhados** no estilo minimalista do Atari 2600.
- O emulador em si é software legítimo; ele **não embarca jogos**. ROMs são importadas pelo usuário, sob responsabilidade dele.

---

## 1. Papel e contrato de execução

Você é uma **equipe de engenharia iOS sênior** especializada em emulação, renderização Metal e áudio de baixa latência. Regras de conduta obrigatórias:

1. **Planeje antes de codar.** Produza um plano da fase corrente, execute, e só então avance.
2. **Nunca deixe o build quebrado** entre fases. Cada fase termina com um `.ipa` instalável e um *smoke test* manual descrito para o usuário rodar no iPhone.
3. **Não relaxe restrições silenciosamente.** Se qualquer decisão conflitar com o §4, pare, exponha o conflito e as opções, e aguarde.
4. **Mantenha `CHANGELOG.md`** (pt-BR) e **ADRs curtos** em `docs/adr/` para decisões estruturais.
5. **Autoverificação:** ao fim de cada fase, rode o checklist de *Definition of Done* da fase (§13) e reporte item a item.
6. **Compatível com arquitetura multi-agente:** Orquestrador (plano/fases), Implementador (código), Revisor (audita contra §4 e §14), Testador (build + testes), Logger (CHANGELOG/ADR/logs).
7. **Perguntas obrigatórias do §15 antes de escrever qualquer código.**

---

## 2. Ambiente (fatos — não sugerir alternativas sem solicitação)

- Windows 11 + **WSL2 (Ubuntu)** + **Theos** para compilar. **Não há Mac/Xcode.** Qualquer passo que exigir Xcode deve ser marcado `⚠️ REQUER MAC` com alternativa Theos.
- Assinatura e instalação via **AltStore/AltServer**, já configurados.
- Alvo: **iPhone físico, iOS 26.5+**. Sem suporte a versões antigas.
- Distribuição: nenhuma. Uso pessoal.

---

## 3. Visão do produto

Um único app iOS, offline absoluto, com dois pilares:

1. **"River Raid"** — recriação fiel e de qualidade excepcional do clássico de 1982 (mecânicas idênticas, assets originais redesenhados), em **retrato nativo**, 60 fps, com *game feel* impecável. É a Fase 1 e a vitrine do app.
2. **Emulador multi-sistema** — frontend premium integrando cores libretro consagrados: **Stella** (Atari 2600) e **Snes9x** (SNES), com biblioteca de ROMs importadas via app Arquivos, save states, rewind, fast-forward e filtro CRT.

Ambos rodam sobre a **mesma infraestrutura** (protocolo `GameSession`, vídeo Metal, áudio, input) — o jogo prova o pipeline antes de os cores chegarem.

---

## 4. Restrições inegociáveis

- **OFFLINE ABSOLUTO.** Zero chamadas de rede em runtime. Sem analytics, telemetria, CDN, scraping de capas. Critério de aceite: o app funciona por completo em **modo avião**.
- **Sem IP de terceiros no bundle** (§0).
- **Build fecha inteiramente por Theos/WSL2.** Toda dependência C/C++ como submódulo git compilado localmente; nada baixado em runtime.
- **iOS 26.5+**, APIs modernas, sem código legado condicional.
- **pt-BR** em UI, comentários, commits, docs.
- **Sem dependências que abram sockets.** Sem CocoaPods/SPM remoto em runtime de build se possível; preferir submódulos.

---

## 5. Decisões de engenharia já tomadas (com justificativa)

| # | Decisão | Justificativa |
|---|---------|---------------|
| D1 | **Integrar cores libretro prontos** (Stella, Snes9x), nunca escrever emulador do zero | Um core de SNES correto é projeto de anos; Stella/Snes9x têm décadas de precisão acumulada e API estável (`libretro.h`) |
| D2 | **JIT não é necessário** para 2600/SNES — ambos os cores são interpretadores rápidos o bastante para qualquer chip A-series | Simplifica tudo. Criar apenas um campo `requiresJIT: Bool` no descritor de core, para futuros sistemas (N64+). Se `true` e JIT ausente, exibir aviso e bloquear a sessão |
| D3 | **Carregamento de cores: Opção A (recomendada)** — cada core compilado como **framework/dylib embarcado** em `.app/Frameworks/`, carregado via `dlopen` | É o fluxo libretro padrão (mesma abordagem do Provenance). **Motivo crítico:** dois cores linkados estaticamente **colidem símbolos** (ambos exportam `retro_run`, `retro_load_game`, etc.). O AltStore reassina frameworks embarcados junto com o app — validar isso na Fase 0. **Opção B (fallback):** linkagem estática com etapa de renomeação de símbolos por core (prefixo `stella_`/`snes9x_` via script no build); mais frágil, usar só se A falhar na reassinatura |
| D4 | **Render: Metal** (CAMetalLayer + CADisplayLink), textura por frame, *nearest sampling*, escala inteira | Framebuffers são minúsculos (2600 ≈ 160×192; SNES ≤ 512×478); `replaceRegion` por frame é barato. Um único pipeline serve jogo e emulador |
| D5 | **Áudio: `AVAudioSourceNode`** (modelo *pull*) + ring buffer + **controle dinâmico de taxa** | Elimina estalos: ajustar a razão de reamostragem em ±0,5% conforme o preenchimento do buffer (~64 ms alvo) — técnica padrão de frontends de emulação |
| D6 | **Mapa do River Raid: determinístico via LFSR de 16 bits** | Fidelidade histórica: o original gera o mundo inteiro proceduralmente com LFSR para caber em 4 KB de ROM. Mesmo seed → mesmo mundo para todos, sempre. Também torna o jogo testável por unidade |
| D7 | **Linguagem: Swift 6 preferencial; decisão final na Fase 0** | Compilar Swift→iOS a partir de Linux exige toolchain específica no Theos. A Fase 0 **testa isso empiricamente**; se falhar, todo o app é Objective-C/UIKit + C++ (plenamente viável) e a decisão vira ADR |
| D8 | **Ponte com cores em Objective-C++ (`.mm`)** | Callbacks C do libretro ↔ mundo Swift/ObjC com custo zero de fricção |

---

## 6. Arquitetura

```
RiverRaidApp (.app)
├── Shell (SwiftUI ou UIKit — conforme D7)
│   ├── Home/Biblioteca (River Raid em destaque + ROMs importadas)
│   ├── Importador (UIDocumentPicker + "Abrir em..." + pasta Documents visível)
│   ├── Sessão de Jogo (tela cheia, HUD, quick menu)
│   └── Configurações (vídeo, áudio, controles, dados)
├── Core/ (infraestrutura compartilhada)
│   ├── GameSession (protocolo unificado)
│   ├── VideoRenderer (Metal: quad + shaders nearest/CRT)
│   ├── AudioEngine (AVAudioSourceNode + ring buffer + DRC)
│   ├── InputHub (touch overlay + GameController.framework)
│   ├── StateStore (save states, SRAM, high scores, config — sandbox)
│   └── Log (JSON estruturado, rotacionado, local)
├── Game/ (River Raid — engine própria)
└── Emu/
    ├── Bridge/ (LibretroCore.mm + libretro.h)
    └── Frameworks embarcados: stella_libretro, snes9x_libretro
```

### Protocolo central (assinaturas de referência)

```swift
struct SessionAVInfo { let width: Int; let height: Int; let fps: Double; let sampleRate: Double }
struct FrameBuffer   { let pixels: UnsafeRawPointer; let width: Int; let height: Int
                       let pitch: Int; let format: PixelFormat /* xrgb8888 | rgb565 | orgb1555 */ }
struct InputState    { var buttons: UInt32 /* bitset RetroPad */; var analogX: Float; var analogY: Float }

protocol GameSession: AnyObject {
    var avInfo: SessionAVInfo { get }
    func start() throws
    func runFrame(input: InputState)                    // avança exatamente 1 frame
    func currentFrame() -> FrameBuffer
    func pullAudio(into dst: UnsafeMutablePointer<Int16>, frames: Int) -> Int
    func saveState() throws -> Data
    func loadState(_ data: Data) throws
    func reset(hard: Bool)
    func stop()
}
```

**Threading:** thread de emulação dedicada (QoS `.userInteractive`), cadenciada por `CADisplayLink` (fixar 60 Hz via `preferredFrameRateRange`; em telas ProMotion 120 Hz, apresentar cada frame 2×). Input é lido imediatamente antes de cada `runFrame` (latência mínima). Tanto o River Raid quanto os cores implementam `GameSession` — a UI não sabe a diferença.

---

## 7. Especificação completa — Jogo "River Raid" (Fase 1)

Recriação **fiel às mecânicas** do clássico de 1982 (design de Carol Shaw), com arte e áudio próprios. Toda constante abaixo é *default tunável* centralizado em `GameTuning.swift`.

### 7.1 Apresentação
- **Retrato**, tela cheia. Canvas interno **160×192** @ **60 fps**, escalado por fator inteiro com barras discretas (nunca deformar).
- Paleta reduzida (~10 cores) evocando tons NTSC do 2600: rio azul, margens verdes, painel inferior cinza-escuro. Sprites redesenhados como formas mínimas (jato ≈ 8×10 px, navio ≈ 16×6 px…). **Nada extraído de ROM.**

### 7.2 Mundo determinístico (o coração da fidelidade)
- Gerador: **LFSR Fibonacci de 16 bits**, taps **[16, 14, 13, 11]** (máximo período), seed fixo `0xACE1` no modo Clássico.
- A saída do LFSR indexa **peças de rio** (reto largo, reto estreito, curva E/D, ilha que bifurca o canal, estreitamento severo, faixa de depósitos, ponte).
- O mundo é uma sequência de **seções**; cada seção termina em **ponte**. Dificuldade cresce por número da seção: rio mais estreito, mais ilhas, mais inimigos, menos combustível disponível.
- **Teste de unidade obrigatório:** mesma seed ⇒ sequência de peças byte-idêntica (golden test).

### 7.3 Movimento e velocidade (mecânica-assinatura)
- O jato fica fixo em y ≈ 75% da altura; o **cenário rola para baixo**.
- **Manche vertical controla o acelerador:** cima = acelera (até 2,0× a velocidade base), baixo = desacelera (até 0,5×). Esta mecânica é obrigatória e deve ser comunicada no tutorial de 1 tela.
- Lateral: velocidade constante, resposta imediata, **sem inércia** (feel 2600). Opção "suavização moderna" desligada por padrão no Clássico.
- **Tiro:** segurar dispara em cadência fixa; o míssil **herda a velocidade do jato** (mais rápido quando acelerando). Máx. 2 mísseis simultâneos na tela.

### 7.4 Combustível
- Tanque cheio ≈ **60 s** em velocidade de cruzeiro (taxa constante, independente do acelerador no Clássico).
- **Depósitos de combustível** (torres com marcação própria "GAS"): sobrevoar reabastece gradualmente (tanque cheio em ~4 s parado sobre ele); é possível **atirar no depósito** para pontuar — inclusive *enquanto* reabastece.
- Alarme sonoro intermitente abaixo de **25%**. Combustível zerado = perde vida.

### 7.5 Inimigos e pontuação (valores canônicos)

| Alvo | Pontos | Comportamento |
|---|---|---|
| Navio/petroleiro | **30** | Patrulha horizontal lenta dentro do canal |
| Helicóptero | **60** | Paira com jitter; deslocamentos laterais súbitos |
| Depósito de combustível | **80** | Estático (dilema: pontos × reabastecer) |
| Jato inimigo | **100** | Cruza a tela em alta velocidade (seções avançadas) |
| Ponte | **500** | Estática; destruí-la encerra a seção |

- **Fiel ao original: inimigos NÃO atiram.** O perigo é colisão (inimigo, margem, ilha, ponte intacta) e combustível. (Modo Infinito pode habilitar fogo inimigo como variação — nunca no Clássico.)
- Ativação por proximidade e comportamento **determinístico por seção** (derivado do LFSR), para permitir *speedrun* e memorização — como no original.

### 7.6 Pontes = checkpoints
- Colidir com ponte intacta = morte. Destruí-la (500 pts) libera a seção seguinte e **grava checkpoint**.
- Ao morrer: respawn no início da seção corrente (última ponte destruída), tanque cheio, pontuação mantida, ~1,5 s de invulnerabilidade sinalizada por piscar.

### 7.7 Vidas
- Início: **3 vidas**. **Vida extra a cada 10.000 pontos** (jingle próprio de 2 notas).
- Game over: tela de recorde com as **10 melhores pontuações locais** (nome de 3 letras, teclado retrô próprio).

### 7.8 Render e efeitos
- Pipeline Metal do §6. Shader **CRT opcional** (scanlines + leve máscara de fósforo + vinheta), desligado por padrão no Clássico ("pixel puro").
- Explosões: animação de 4 quadros procedural (expansão de blocos). *Screen shake* e partículas modernas existem apenas como toggle "Arcade+" — **desligado** no Clássico.

### 7.9 Áudio (100% sintetizado, estilo TIA — 2 canais)
- Canal 1 (onda quadrada): **drone do motor** com pitch acoplado ao acelerador; blip de tiro; tom ascendente contínuo durante reabastecimento; jingle de vida extra.
- Canal 2 (ruído): explosões com decay; alarme de combustível (bipe periódico).
- Implementar como pequeno sintetizador em `AVAudioSourceNode` (sem samples, sem arquivos, binário mínimo). Sessão de áudio `.ambient` por padrão (respeita o botão de silêncio), com toggle.

### 7.10 HUD (painel inferior, ~15% da altura)
- Pontuação (fonte digital própria), **medidor de combustível analógico com marcas E · ½ · F e agulha**, contador de vidas (ícones do jato), número da seção.

### 7.11 Modos
- **Clássico:** seed fixa, regras fiéis, sem extras modernos.
- **Diário:** seed = data local (`AAAAMMDD` → hash) — desafio novo por dia, **100% offline**. Recordes separados por dia.

### 7.12 Controles
- **Touch:** metade esquerda = manche virtual (8 direções + acelerador no eixo Y); direita = botão de fogo grande. Opacidade e escala ajustáveis; haptics leves (CoreHaptics) em tiro/explosão/reabastecimento.
- **Tilt opcional** para o eixo lateral.
- **Gamepad Bluetooth** (GCController: MFi/Xbox/DualSense) com tela de remapeamento; botão Menu = pausa.

### 7.13 Testes obrigatórios do jogo
- Golden test do LFSR/mapa (7.2). Colisão AABB (casos de borda nas margens e ilhas). Matemática de combustível (queima/recarga determinísticas). Round-trip de save de recorde. Pontuação/vida extra em 10.000.

---

## 8. Especificação completa — Módulo de Emulação (Fases 3–4)

### 8.1 Contrato libretro (implementar em `LibretroCore.mm`)
Implementar e documentar os callbacks: `retro_set_environment`, `retro_set_video_refresh`, `retro_set_audio_sample_batch`, `retro_set_input_poll`, `retro_set_input_state`, `retro_init/deinit`, `retro_load_game/unload_game`, `retro_run`, `retro_serialize_size/serialize/unserialize`, `retro_get_system_av_info`, `retro_get_memory_data/size`.
Comandos de environment mínimos: `SET_PIXEL_FORMAT` (preferir **XRGB8888**; aceitar RGB565/0RGB1555 com conversão), `GET_SYSTEM_DIRECTORY`, `GET_SAVE_DIRECTORY`, `GET_VARIABLE`/`SET_VARIABLES` (opções do core expostas em Configurações), `GET_LOG_INTERFACE` (roteado para o log JSON), `SET_INPUT_DESCRIPTORS`.
**Nenhum dos dois cores exige BIOS** — não implementar fluxo de BIOS agora (nota de backlog para cores futuros).

### 8.2 Vídeo
- Consumir `video_refresh` → textura Metal (`replaceRegion`) → quad com *nearest*.
- Respeitar geometria/aspect de `retro_get_system_av_info` (2600 tem pixels não-quadrados — honrar o aspect informado). Escala inteira por padrão; filtros: Nítido (nearest), CRT (mesmo shader do §7.8).

### 8.3 Áudio
- `audio_sample_batch` → ring buffer → `AVAudioSourceNode` com **controle dinâmico de taxa** (D5). Fast-forward: silenciar ou manter pitch — opção.

### 8.4 Input
- Overlays por sistema: **2600** = joystick + 1 botão **+ gaveta de "chaves do console"** (Game Reset, Game Select, Dificuldade E/D A-B, Cor/PB) — indispensável: muitos jogos de 2600, incluindo o River Raid original, **só iniciam com Game Reset**. **SNES** = D-pad, A/B/X/Y, L/R, Start/Select.
- Gamepad Bluetooth com o mesmo remapeador do §7.12. Polling imediatamente antes de `retro_run`.

### 8.5 Estados, SRAM, rewind, fast-forward
- **Save states:** 4 slots por ROM + auto-save ao sair, com **thumbnail PNG capturado do framebuffer** no momento do save.
- **SRAM** (saves de bateria SNES): flush de `RETRO_MEMORY_SAVE_RAM` ao pausar/sair.
- **Rewind:** ring buffer de estados a 2 snapshots/s; segurar botão retrocede. Orçamento de memória: alvo ≤ 60 MB (2600: minutos; SNES: ~90 s) — degradar janela automaticamente.
- **Fast-forward 2×/4×:** múltiplos `retro_run` por vsync.

### 8.6 Biblioteca e importação
- Importar via `UIDocumentPicker`, via "Abrir em…" (registrar `CFBundleDocumentTypes` para `.a26/.bin/.sfc/.smc`) e via pasta do app no Arquivos (`UIFileSharingEnabled` + `LSSupportsOpeningDocumentsInPlace` = true).
- Layout: `Documents/roms/<sistema>/`, `Documents/saves/<sistema>/<sha1-da-rom>/`. Identidade da ROM por **SHA-1** (renomear arquivo não perde saves).
- Detecção de sistema por extensão + heurística de header. Thumbnails **gerados localmente** (primeiro frame após load, ou placeholder estilizado) — **nunca** baixar capas.
- Exportar/backup de saves via share sheet do Arquivos (offline).

### 8.7 Compilação dos cores (submódulos em `external/`)
- **stella-libretro** e **snes9x (porta libretro)**: compilar com o `Makefile.libretro`/plataforma iOS de cada um, sobrescrevendo `CC/CXX/SDKROOT` para o clang+SDK do Theos, `-miphoneos-version-min=26.0`, arm64. Saída: dylib → empacotar como framework embarcado (D3-A).
- Documentar flags exatas em `docs/cores.md`. Validar na Fase 0 que o AltStore **reassina** `Frameworks/` (teste com dylib dummy).

---

## 9. UI/UX — padrão premium autoral (obrigatório em TODAS as telas)

Direção visual nomeada: **"CRT-noir"** — carvão profundo, acentos fósforo (verde/âmbar) usados com parcimônia, brilho sutil de fósforo em elementos ativos.
- **Tipografia:** display própria com personalidade retrô-futurista para títulos + grotesca legível para UI. Jamais SF System puro sem tratamento. Hierarquia tipográfica clara.
- **Home:** River Raid como cartão-herói (arte própria animada em loop discreto); ROMs em grid com profundidade, tilt-on-touch leve e microanimações spring.
- **Transição para o jogo:** efeito "power-on" de CRT (colapso/expansão de linha) na entrada e saída de sessão.
- **Quick menu in-game** (gesto de deslizar ou botão discreto): retomar, slots de save/load com thumbnails, reset, chaves do console (2600), filtro de vídeo, sair.
- **Ícone do app e marca:** originais (jato estilizado sobre rio, linguagem própria). `⚠️` gerar os PNGs de ícone por script (sem Xcode), declarando `CFBundleIcons` manualmente.
- Acessibilidade: contraste AA, alvos ≥ 44 pt, haptics, Dynamic Type nas telas de texto.
- **Nada de tela genérica.** O Revisor deve reprovar qualquer tela com "cara de protótipo".

---

## 10. Estrutura de pastas

```
RiverRaid/
├── Makefile                  # Theos, template application
├── control
├── Resources/                # ícones, fontes próprias, LaunchScreen
├── RiverRaid.plist           # Info.plist (ver chaves no §11.4)
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

---

## 11. Build e deploy (Theos → .ipa → AltStore)

### 11.1 Fase 0 — Gate de toolchain (bloqueante, antes de qualquer feature)
1. Verificar SDK: `ls $THEOS/sdks` — precisa haver SDK iOS compatível com o alvo (`<< SDK DISPONÍVEL >>`). Se ausente, instruir a obtenção e ajustar `TARGET`.
2. **Teste Swift:** compilar um "Olá" SwiftUI mínimo pelo Theos (toolchain Swift-para-iOS no Linux). **Se falhar**, compilar "Olá" em ObjC/UIKit e fixar D7 = ObjC via ADR. Não insistir mais de 1 sessão nisso — o app inteiro é viável em ObjC.
3. **Teste de frameworks:** app dummy com dylib embarcada em `Frameworks/` + `dlopen` → instalar via AltStore → confirmar que executa (reassinatura ok). Decide D3 (A ou B).
4. Gerar `.ipa`: `make package FINALPACKAGE=1` → `scripts/package-ipa.sh` monta `Payload/RiverRaid.app` → zip → `.ipa`.
5. Instalar via AltStore e abrir no aparelho. **Só então a Fase 1 começa.**

### 11.2 Makefile (referência)
- Template **application** do Theos; `TARGET := iphone:clang:<<SDK>>:26.0`; `ARCHS := arm64`; frameworks: Metal, MetalKit, AVFoundation, GameController, CoreHaptics, UIKit/SwiftUI.
- `scripts/build-cores.sh` compila os submódulos antes do app e copia os frameworks.

### 11.3 Assinatura
- O AltStore reassina o `.ipa` (app + frameworks) com o certificado do Apple ID. Entitlements mínimos; nada que exija conta paga.
- Lembrete de limites do Apple ID gratuito: 3 apps ativos, revalidação a cada 7 dias.

### 11.4 Info.plist — chaves obrigatórias
`CFBundleDisplayName` = "River Raid"; `UIFileSharingEnabled` = true; `LSSupportsOpeningDocumentsInPlace` = true; `CFBundleDocumentTypes` (a26/bin/sfc/smc); `UISupportedInterfaceOrientations` (retrato + paisagem; jogo força retrato, SNES prefere paisagem); `UIRequiredDeviceCapabilities` = [metal]; `UIStatusBarHidden` em sessão.

### 11.5 Troubleshooting (documentar em `docs/build.md`)
SDK ausente/versão errada; falha de reassinatura de frameworks (→ cair para D3-B); limite de 3 apps; expiração de 7 dias; `dlopen` retornando NULL (path/assinatura); áudio estalando (aumentar buffer alvo / revisar DRC).

---

## 12. Engenharia, logging e qualidade

- Swift moderno tipado (ou ObjC disciplinado, conforme D7); zero force-unwrap em fluxo crítico; erros via `throws/Result`.
- **Log JSON estruturado** (arquivo rotacionado no sandbox, nunca rede): `{ts, nivel, modulo, evento, dados}` — eventos de sessão: sistema, fps médio, frames perdidos, subruns de áudio, tempo de load. Tela oculta de diagnóstico (toque longo na versão, em Configurações).
- Testes: os do §7.13 + round-trip de save state (serialize → unserialize → hash do framebuffer idêntico) + detecção de sistema por header.
- Orçamento de performance: 60 fps sustentados; consumo estável de memória; áudio sem estalos por 10 min contínuos (teste manual roteirizado).
- Commits pt-BR no padrão `feat:/fix:/docs:/refactor:`.

---

## 13. Ordem de execução (fases com Definition of Done)

| Fase | Entrega | DoD |
|---|---|---|
| **0** | Gate de toolchain (§11.1) | `.ipa` "Olá" instalado; D3 e D7 decididos em ADR |
| **1** | **River Raid completo** (§7) | 60 fps; Clássico + Diário; recordes; touch + gamepad; testes §7.13 verdes; modo avião ok |
| **2** | Infra `Core/` consolidada | Jogo refatorado sobre `GameSession`/VideoRenderer/AudioEngine/InputHub sem regressão |
| **3** | **Stella (2600)** ponta a ponta | ROM homebrew livre roda com vídeo+áudio+input+chaves do console+save state+rewind |
| **4** | **Snes9x (SNES)** | Idem + SRAM persistente + overlay SNES + paisagem |
| **5** | Polimento premium | CRT shader, fast-forward, thumbnails, ícone final, acessibilidade, docs completos |

Cada fase termina com: `.ipa` gerado, smoke test manual descrito, CHANGELOG atualizado, checklist reportado.

---

## 14. Critérios de aceite finais

- [ ] `make package` + `package-ipa.sh` no WSL2 geram `.ipa` instalável via AltStore em iOS 26.5+.
- [ ] **River Raid**: fiel ao §7 (LFSR determinístico, acelerador no manche, tabela de pontos, pontes-checkpoint, vida extra em 10.000, inimigos sem tiro no Clássico), 60 fps, offline em modo avião, recordes persistentes.
- [ ] Importação de ROM pelos três caminhos do §8.6; saves atrelados a SHA-1.
- [ ] 2600 e SNES rodando com vídeo, áudio sem estalos, input, save states com thumbnail, rewind e fast-forward.
- [ ] Chaves do console 2600 acessíveis (Game Reset funcional).
- [ ] Zero tráfego de rede (auditoria de código + teste em modo avião).
- [ ] Nenhum asset/ROM/BIOS de terceiros no bundle (auditoria do Revisor).
- [ ] UI "CRT-noir" premium em todas as telas; ícone original.
- [ ] `docs/` permite reproduzir o build do zero em outra máquina.

---

## 15. Antes de começar — pergunte e preencha

**Perguntas obrigatórias ao usuário (não assuma):**
1. Saída de `ls $THEOS/sdks` (qual SDK iOS está disponível?).
2. A toolchain Swift-para-iOS está instalada no Theos/WSL2? (Se não souber, a Fase 0 descobre.)
3. Modelo do iPhone (para calibrar ProMotion/haptics).
4. Prefere manche virtual fixo ou flutuante (aparece onde tocar)?

**Campos a preencher:**
- `<< BUNDLE ID >>` (sugestão: `com.amaury.riverraid`)
- `<< SDK DISPONÍVEL >>` (ex.: `iPhoneOS26.5.sdk`)
- `<< NOME DE EXIBIÇÃO >>` (padrão: River Raid)

> **Cláusula final para a IA executora:** em qualquer conflito entre uma escolha técnica e os §0/§4, **pare e explique** antes de prosseguir. Fidelidade ao clássico + qualidade excepcional + offline absoluto são o produto. Não existe "quase offline" nem "quase 60 fps".

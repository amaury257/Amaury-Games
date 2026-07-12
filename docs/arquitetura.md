# Arquitetura

> Visão de implementação da Fase 1. O desenho de referência completo está no
> §6 do `prompt-mestre.md`.

## Camadas

```
Shell (ObjC/UIKit)        Home · Jogo · Recordes · Diagnóstico (Fase 0)
Core  (ObjC)              GameSession (protocolo) · VideoRenderer (Metal)
                          AudioEngine (AVAudioSourceNode) · InputHub · EmuLoop
                          StateStore · Log
Game  (ObjC)              RiverRaidSession — adapta o motor C ao GameSession
Game/engine (C puro)      rr_jogo · rr_mundo · rr_render · rr_audio · rr_recordes
Emu   (Fases 3–4)         LibretroCore.mm + cores como dylibs (D3)
```

## Por que o motor é C puro

- **Determinismo testável**: mesma seed + mesmas entradas ⇒ mesmo framebuffer,
  frame a frame. Os testes obrigatórios (§7.13) compilam e rodam em qualquer
  host (`scripts/rodar-testes.sh`), sem Theos nem aparelho.
- **Save state trivial**: `rr_jogo` é POD sem ponteiros — serializar é memcpy
  com cabeçalho versionado.
- **Render por software**: o jogo desenha num canvas 160×192 XRGB8888, como um
  core libretro faz. A camada Metal (VideoRenderer) só sobe a textura e escala
  por fator inteiro — exatamente o pipeline que os cores usarão nas Fases 3–4.

## Threading

- **Thread de emulação** (EmuLoop): NSThread QoS userInteractive + CADisplayLink
  com `preferredFrameRateRange` fixo em 60 Hz. Por tique: lê o InputHub →
  `executarQuadro` → `apresentarQuadro`. iPhone 14 = tela 60 Hz (1:1).
- **Thread de áudio**: AVAudioSourceNode (pull) chama `puxarAudio` da sessão;
  o sintetizador é clocado pelo próprio callback — sem deriva, sem estalos.
  O ring buffer + controle dinâmico de taxa (D5) entra na Fase 3, quando os
  cores libretro passarem a *empurrar* lotes de áudio.
- **Main thread**: UI, pausa, fim de jogo, haptics (timer de 10 Hz coleta
  eventos da sessão).
- Sincronização: um único `os_unfair_lock` por sessão protege jogo/áudio/framebuffer.

## Decisões que valem lembrar

- Shaders Metal compilados **em runtime** (`newLibraryWithSource`) — o Theos
  no Linux não tem o compilador offline de shaders. Fonte embutida no
  VideoRenderer.m.
- Canvas XRGB8888 little-endian casa com `MTLPixelFormatBGRA8Unorm` sem swizzle.
- Recordes: structs C serializados (`rr_recordes`), persistidos pelo StateStore
  em `Documents/recordes/<modo>.bin`; Diário separado por dia (`diario-AAAAMMDD`).
- Log JSON por linha em `Documents/logs/app.log`, rotação em 256 KB, nunca rede.

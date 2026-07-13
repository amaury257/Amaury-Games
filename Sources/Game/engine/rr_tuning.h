// rr_tuning.h — Constantes de gameplay do River Raid (§7 do prompt mestre).
// Todos os valores são defaults tunáveis; o modo Clássico usa exatamente estes.
#ifndef RR_TUNING_H
#define RR_TUNING_H

// Canvas interno (§7.1): 160×192, escala inteira feita pelo renderizador.
#define RR_LARGURA        160
#define RR_ALTURA         192
#define RR_ALTURA_JOGO    162                 // área de jogo; o restante é HUD (§7.10)
#define RR_ALTURA_HUD     (RR_ALTURA - RR_ALTURA_JOGO)
#define RR_FPS            60

// Áudio sintetizado (§7.9)
#define RR_TAXA_AUDIO           48000
#define RR_AMOSTRAS_POR_QUADRO  (RR_TAXA_AUDIO / RR_FPS)

// Jato (fixo em y ≈ 75% da área de jogo; o cenário rola — §7.3)
#define RR_JATO_LARG      8
#define RR_JATO_ALT       10
#define RR_JATO_Y         121                 // linha de tela do topo do sprite
// offset do centro do jato em linhas de mundo relativo à rolagem
#define RR_JATO_MUNDO_DY  ((float)(RR_ALTURA_JOGO - 1 - RR_JATO_Y) - RR_JATO_ALT * 0.5f)

// Movimento (§7.3): manche vertical = acelerador; lateral sem inércia
// Valores ajustados após teste no aparelho (relato: rolagem rápida demais,
// manche lateral exagerado — ver docs/CHANGELOG.md).
#define RR_VEL_BASE       0.95f               // rolagem px/frame em cruzeiro
#define RR_ACEL_MIN       0.5f
#define RR_ACEL_MAX       2.0f
#define RR_VEL_LATERAL    1.3f

// Tiro (§7.3): cadência fixa, míssil herda a velocidade do jato
#define RR_MAX_MISSEIS    2
#define RR_MISSIL_VEL     4.0f                // + velocidade de rolagem do jato
#define RR_CADENCIA       12                  // frames entre tiros

// Combustível (§7.4)
#define RR_TANQUE_MAX     100.0f
#define RR_QUEIMA         (RR_TANQUE_MAX / (60.0f * RR_FPS))  // tanque ≈ 60 s
#define RR_RECARGA        (RR_TANQUE_MAX / (4.0f * RR_FPS))   // cheio em ≈ 4 s
#define RR_ALARME_LIMIAR  25.0f

// Vidas e pontuação (§7.5/§7.7 — valores canônicos)
#define RR_VIDAS_INICIAIS 3
#define RR_VIDA_EXTRA     10000
#define RR_PTS_NAVIO      30
#define RR_PTS_HELI       60
#define RR_PTS_DEPOSITO   80
#define RR_PTS_JATO       100
#define RR_PTS_PONTE      500

// Respawn (§7.6): início da seção, tanque cheio, ~1,5 s invulnerável
#define RR_INVULN_FRAMES  90
#define RR_EXPLOSAO_FRAMES 36

// Mundo determinístico (§7.2 / D6)
#define RR_SEED_CLASSICO  0xACE1
#define RR_TILE_ALTURA    32                  // linhas de mundo por peça
#define RR_TILES_POR_SECAO 20                 // última peça é sempre a ponte
#define RR_MAX_ENTS       48

#endif

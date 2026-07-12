// rr_jogo.h — Motor do River Raid em C puro (portável, determinístico, testável).
// O struct rr_jogo é POD sem ponteiros: save state = memcpy versionado.
#ifndef RR_JOGO_H
#define RR_JOGO_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "rr_tuning.h"
#include "rr_lfsr.h"

// ---------------------------------------------------------------- entrada
typedef struct {
    float eixo_x;   // -1..1 (esquerda..direita); sem inércia (§7.3)
    float eixo_y;   // -1..1; cima acelera até 2,0×, baixo desacelera até 0,5×
    bool  fogo;
} rr_entrada;

// ---------------------------------------------------------------- mundo (§7.2)
typedef enum {
    RR_TILE_RETO_LARGO = 0, RR_TILE_RETO_ESTREITO, RR_TILE_CURVA_E, RR_TILE_CURVA_D,
    RR_TILE_ILHA, RR_TILE_SEVERO, RR_TILE_DEPOSITOS, RR_TILE_PONTE
} rr_tile_tipo;

typedef struct { float centro, meia; } rr_borda;    // fronteira entre peças
typedef struct { uint8_t tipo; float ilha_meia; } rr_tile;

// ---------------------------------------------------------------- entidades (§7.5)
typedef enum {
    RR_ENT_LIVRE = 0, RR_ENT_NAVIO, RR_ENT_HELI, RR_ENT_DEPOSITO, RR_ENT_JATO, RR_ENT_PONTE
} rr_ent_tipo;

typedef struct {
    uint8_t  tipo, vivo, ativo;
    float    x, y;      // centro; y em linhas de mundo (crescem rio acima)
    float    vx;
    uint16_t rnd;       // PRNG por entidade (comportamento determinístico)
} rr_ent;

typedef enum { RR_FASE_JOGANDO = 0, RR_FASE_EXPLODINDO, RR_FASE_FIM } rr_fase;

// ---------------------------------------------------------------- estado completo
typedef struct {
    uint16_t seed;
    uint8_t  diario;
    uint16_t lfsr, lfsr_secao;      // corrente e snapshot do início da seção (respawn)
    uint32_t secao;                 // 1-based
    float    rolagem, rolagem_secao;

    // geometria da seção corrente e da anterior (linhas ainda visíveis na tela)
    rr_tile  tiles[RR_TILES_POR_SECAO];
    rr_borda bordas[RR_TILES_POR_SECAO + 1];
    rr_tile  tiles_ant[RR_TILES_POR_SECAO];
    rr_borda bordas_ant[RR_TILES_POR_SECAO + 1];

    rr_ent   ents[RR_MAX_ENTS];

    float    jato_x, acel;
    uint8_t  fase;
    int16_t  invuln, explosao_t, cadencia;
    struct { uint8_t ativo; float x, y, vy; } misseis[RR_MAX_MISSEIS];

    float    combustivel;
    int32_t  vidas;
    uint32_t pontos, proxima_extra;

    // explosões visuais (mísseis acertando alvos)
    struct { uint8_t t; float x, y; } fx[8];

    // eventos/estados lidos pela camada de áudio (§7.9) — consumidos pela sessão
    uint8_t  ev_tiro, ev_explosao, ev_vida_extra;
    uint8_t  reabastecendo, alarme;

    uint32_t frame;
} rr_jogo;

// ---------------------------------------------------------------- API
void     rr_jogo_iniciar(rr_jogo *j, uint16_t seed, bool diario);
void     rr_jogo_quadro(rr_jogo *j, const rr_entrada *e);
uint16_t rr_seed_diario(uint32_t aaaammdd);   // Diário: AAAAMMDD → hash 16 bits

// mundo: margens do canal na linha de mundo y (ilha_e/ilha_d só valem se ilha_d > ilha_e)
void rr_mundo_margens(const rr_jogo *j, float y, float *esq, float *dir,
                      float *ilha_e, float *ilha_d);
void rr_mundo_gerar_secao(rr_jogo *j);

// save state (§12): cabeçalho versionado + memcpy do struct POD
size_t rr_estado_tamanho(void);
size_t rr_estado_salvar(const rr_jogo *j, void *dst, size_t cap);
bool   rr_estado_carregar(rr_jogo *j, const void *src, size_t len);

#endif

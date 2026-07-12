// rr_mundo.c — Geração determinística do rio (§7.2 / D6).
// Cada seção = RR_TILES_POR_SECAO peças; a última é sempre a ponte.
// A dificuldade cresce com o número da seção: rio mais estreito, mais ilhas,
// mais inimigos. Mesma seed ⇒ sequência de peças byte-idêntica (golden test).
#include "rr_jogo.h"

static float clampf(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }

// Escolha da peça i conforme LFSR e dificuldade d.
// Regras fixas: peça 0 mantém a geometria (transição suave pós-ponte),
// peça i%7==3 é faixa de depósitos (combustível garantido em cadência),
// última peça é a ponte.
static rr_tile_tipo escolher_tipo(uint16_t v, uint32_t d, int i) {
    if (i == RR_TILES_POR_SECAO - 1) return RR_TILE_PONTE;
    if (i == 0) return RR_TILE_RETO_LARGO;
    if (i % 7 == 3) return RR_TILE_DEPOSITOS;
    uint16_t r = v % 100;
    uint16_t p_ilha = (uint16_t)(12 + 2 * (d > 10 ? 10 : d));
    uint16_t p_severo = (uint16_t)(d >= 2 ? 4 + (d > 12 ? 12 : d) : 0);
    if (r < p_ilha) return RR_TILE_ILHA;
    r = (uint16_t)(r - p_ilha);
    if (r < p_severo) return RR_TILE_SEVERO;
    r = (uint16_t)(r - p_severo);
    if (r < 18) return RR_TILE_CURVA_E;
    if (r < 36) return RR_TILE_CURVA_D;
    if (r < 60) return RR_TILE_RETO_ESTREITO;
    return RR_TILE_RETO_LARGO;
}

static int slot_livre(rr_jogo *j) {
    for (int k = 0; k < RR_MAX_ENTS; k++)
        if (j->ents[k].tipo == RR_ENT_LIVRE) return k;
    return -1;
}

static void criar_ent(rr_jogo *j, rr_ent_tipo tipo, float x, float y, float vx, uint16_t rnd) {
    int k = slot_livre(j);
    if (k < 0) return;
    rr_ent *e = &j->ents[k];
    e->tipo = (uint8_t)tipo; e->vivo = 1; e->ativo = 0;
    e->x = x; e->y = y; e->vx = vx; e->rnd = rnd ? rnd : 1;
}

// Interpola as bordas da seção nas peças [tiles/bordas] para a linha local yl (>=0).
static void margens_secao(const rr_tile *tiles, const rr_borda *bordas, float yl,
                          float *esq, float *dir, float *ie, float *id) {
    int t = (int)(yl / RR_TILE_ALTURA);
    float frac;
    if (t >= RR_TILES_POR_SECAO) { t = RR_TILES_POR_SECAO - 1; frac = 1.0f; }
    else frac = (yl - (float)t * RR_TILE_ALTURA) / (float)RR_TILE_ALTURA;
    const rr_borda *a = &bordas[t], *b = &bordas[t + 1];
    float centro = a->centro + (b->centro - a->centro) * frac;
    float meia   = a->meia + (b->meia - a->meia) * frac;
    *esq = centro - meia;
    *dir = centro + meia;
    // ilha: rampa nos 25% iniciais/finais da peça, platô no meio
    float ilha = tiles[t].ilha_meia;
    if (ilha > 0.5f) {
        float rampa = frac < 0.5f ? frac : 1.0f - frac;
        float fator = rampa * 4.0f; if (fator > 1.0f) fator = 1.0f;
        float im = ilha * fator;
        if (im > 0.5f) { *ie = centro - im; *id = centro + im; return; }
    }
    *ie = 0.0f; *id = 0.0f;   // sem ilha (id <= ie)
}

void rr_mundo_margens(const rr_jogo *j, float y, float *esq, float *dir,
                      float *ie, float *id) {
    float yl = y - j->rolagem_secao;
    if (yl >= 0.0f) {
        margens_secao(j->tiles, j->bordas, yl, esq, dir, ie, id);
    } else {
        // linhas da seção anterior ainda visíveis na tela
        float ylp = yl + (float)(RR_TILES_POR_SECAO * RR_TILE_ALTURA);
        if (ylp < 0.0f) ylp = 0.0f;
        margens_secao(j->tiles_ant, j->bordas_ant, ylp, esq, dir, ie, id);
    }
}

void rr_mundo_gerar_secao(rr_jogo *j) {
    uint32_t d = j->secao > 1 ? j->secao - 1 : 0;
    if (d > 16) d = 16;

    // geometria: bordas[0] já contém a continuidade (fim da seção anterior)
    for (int i = 0; i < RR_TILES_POR_SECAO; i++) {
        uint16_t v = rr_lfsr_prox(&j->lfsr);
        rr_tile_tipo t = escolher_tipo(v, d, i);
        rr_borda a = j->bordas[i], b = a;
        float ilha = 0.0f;
        float larga = clampf(52.0f - 1.0f * (float)d, 34.0f, 52.0f);
        float estreita = clampf(36.0f - 1.0f * (float)d, 24.0f, 36.0f);
        switch (t) {
        case RR_TILE_RETO_LARGO:
            if (i > 0) { b.meia = larga; b.centro += (80.0f - b.centro) * 0.5f; }
            break;
        case RR_TILE_RETO_ESTREITO: b.meia = estreita; break;
        case RR_TILE_CURVA_E: b.centro -= 12.0f + (float)((v >> 8) % 10); break;
        case RR_TILE_CURVA_D: b.centro += 12.0f + (float)((v >> 8) % 10); break;
        case RR_TILE_ILHA:
            if (b.meia < 44.0f) b.meia = 44.0f;
            ilha = 8.0f + (float)(v % 5) + 0.5f * (float)d;
            if (ilha > b.meia - 14.0f) ilha = b.meia - 14.0f;
            break;
        case RR_TILE_SEVERO: b.meia = clampf(22.0f - 0.4f * (float)d, 15.0f, 22.0f); break;
        case RR_TILE_DEPOSITOS: if (b.meia < 40.0f) b.meia = 40.0f; break;
        case RR_TILE_PONTE: b.centro = 80.0f; b.meia = 40.0f; break;
        }
        b.meia = clampf(b.meia, 14.0f, 70.0f);
        b.centro = clampf(b.centro, 8.0f + b.meia, 152.0f - b.meia);
        j->tiles[i].tipo = (uint8_t)t;
        j->tiles[i].ilha_meia = ilha;
        j->bordas[i + 1] = b;
    }

    // entidades: peças 1..N-1 (a peça 0 é o respiro pós-ponte)
    float y0 = j->rolagem_secao;
    for (int i = 1; i < RR_TILES_POR_SECAO; i++) {
        rr_tile_tipo t = (rr_tile_tipo)j->tiles[i].tipo;
        float ty = y0 + (float)i * RR_TILE_ALTURA;

        if (t == RR_TILE_PONTE) {
            criar_ent(j, RR_ENT_PONTE, 80.0f, ty + RR_TILE_ALTURA * 0.5f, 0.0f,
                      rr_lfsr_prox(&j->lfsr));
            continue;
        }
        if (t == RR_TILE_DEPOSITOS) {
            uint16_t v = rr_lfsr_prox(&j->lfsr);
            int n = 2 + (int)(v % 2u);
            for (int q = 0; q < n; q++) {
                float ey = ty + 5.0f + (float)q * (RR_TILE_ALTURA - 10) / (float)n;
                float esq, dir, ie, id;
                rr_mundo_margens(j, ey, &esq, &dir, &ie, &id);
                float frac = 0.25f + 0.5f * (float)((v >> (2 * q)) % 16u) / 16.0f;
                criar_ent(j, RR_ENT_DEPOSITO, esq + frac * (dir - esq), ey, 0.0f,
                          (uint16_t)(v + q));
            }
            continue;
        }

        // inimigos comuns: quantidade cresce com a dificuldade
        uint16_t v = rr_lfsr_prox(&j->lfsr);
        int max_n = 1 + (int)(d > 6 ? 3 : d / 2 + 1);
        int n = (int)(v % (uint16_t)(max_n + 1));
        for (int q = 0; q < n; q++) {
            uint16_t w = rr_lfsr_prox(&j->lfsr);
            float ey = ty + 4.0f + (float)(w % (RR_TILE_ALTURA - 8));
            float esq, dir, ie, id;
            rr_mundo_margens(j, ey, &esq, &dir, &ie, &id);
            int tipo_r = (int)((w >> 6) % 3u);
            if (tipo_r == 2 && j->secao < 3) tipo_r = 0;   // jato inimigo só em seções avançadas
            if (tipo_r == 0) {          // navio: patrulha lenta no canal
                float frac = 0.2f + 0.6f * (float)((w >> 9) % 8u) / 8.0f;
                float vel = (0.25f + 0.02f * (float)d) * (((w >> 5) & 1u) ? 1.0f : -1.0f);
                criar_ent(j, RR_ENT_NAVIO, esq + frac * (dir - esq), ey, vel, w);
            } else if (tipo_r == 1) {   // helicóptero: paira com jitter
                float frac = 0.25f + 0.5f * (float)((w >> 9) % 8u) / 8.0f;
                criar_ent(j, RR_ENT_HELI, esq + frac * (dir - esq), ey, 0.0f, w);
            } else {                    // jato inimigo: cruza a tela em alta velocidade
                float vel = (2.2f + 0.1f * (float)d) * (((w >> 5) & 1u) ? 1.0f : -1.0f);
                criar_ent(j, RR_ENT_JATO, vel > 0 ? -12.0f : 172.0f, ey, vel, w);
            }
        }
    }
}

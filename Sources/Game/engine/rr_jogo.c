// rr_jogo.c — Máquina de estados do River Raid (§7.3–§7.7).
// Tudo determinístico: mesma seed + mesmas entradas ⇒ mesmo jogo, frame a frame.
#include "rr_jogo.h"
#include <string.h>

static float clampf(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }

// AABB por centro/meias-dimensões; bordas exatamente encostadas NÃO colidem.
static bool aabb(float ax, float ay, float amx, float amy,
                 float bx, float by, float bmx, float bmy) {
    return (ax - amx < bx + bmx) && (bx - bmx < ax + amx) &&
           (ay - amy < by + bmy) && (by - bmy < ay + amy);
}

// meias-dimensões (largura, altura em linhas de mundo) por tipo de entidade
static void ent_dim(const rr_jogo *j, const rr_ent *e, float *mx, float *my) {
    switch ((rr_ent_tipo)e->tipo) {
    case RR_ENT_NAVIO:    *mx = 8.0f;  *my = 3.0f; break;
    case RR_ENT_HELI:     *mx = 6.0f;  *my = 4.0f; break;
    case RR_ENT_DEPOSITO: *mx = 7.0f;  *my = 8.0f; break;
    case RR_ENT_JATO:     *mx = 5.0f;  *my = 3.0f; break;
    case RR_ENT_PONTE: {
        float esq, dir, ie, id;
        rr_mundo_margens(j, e->y, &esq, &dir, &ie, &id);
        *mx = (dir - esq) * 0.5f; *my = 5.0f; break;
    }
    default: *mx = 0; *my = 0; break;
    }
}

static uint32_t pontos_ent(rr_ent_tipo t) {
    switch (t) {
    case RR_ENT_NAVIO: return RR_PTS_NAVIO;
    case RR_ENT_HELI: return RR_PTS_HELI;
    case RR_ENT_DEPOSITO: return RR_PTS_DEPOSITO;
    case RR_ENT_JATO: return RR_PTS_JATO;
    case RR_ENT_PONTE: return RR_PTS_PONTE;
    default: return 0;
    }
}

uint16_t rr_seed_diario(uint32_t aaaammdd) {
    uint32_t x = aaaammdd;
    x ^= x >> 16; x *= 0x7feb352dU;
    x ^= x >> 15; x *= 0x846ca68bU;
    x ^= x >> 16;
    uint16_t s = (uint16_t)(x & 0xFFFFu);
    return s ? s : (uint16_t)RR_SEED_CLASSICO;   // LFSR não pode iniciar em 0
}

static void adicionar_fx(rr_jogo *j, float x, float y) {
    for (int i = 0; i < 8; i++)
        if (j->fx[i].t == 0) { j->fx[i].t = 24; j->fx[i].x = x; j->fx[i].y = y; return; }
}

static void pontuar(rr_jogo *j, uint32_t pts) {
    j->pontos += pts;
    if (j->pontos >= j->proxima_extra) {        // vida extra a cada 10.000 (§7.7)
        j->vidas++;
        j->ev_vida_extra = 1;
        j->proxima_extra += RR_VIDA_EXTRA;
    }
}

// (Re)posiciona o jato no início da seção corrente, regenerando-a do snapshot
// do LFSR — o layout renasce idêntico (checkpoint da última ponte, §7.6).
static void entrar_na_secao(rr_jogo *j) {
    j->lfsr = j->lfsr_secao;
    // remove entidades da seção corrente (as da anterior, abaixo, já não importam)
    for (int k = 0; k < RR_MAX_ENTS; k++)
        if (j->ents[k].y >= j->rolagem_secao) j->ents[k].tipo = RR_ENT_LIVRE;
    rr_mundo_gerar_secao(j);
    j->rolagem = j->rolagem_secao;
    float esq, dir, ie, id;
    rr_mundo_margens(j, j->rolagem + RR_JATO_MUNDO_DY, &esq, &dir, &ie, &id);
    j->jato_x = (esq + dir) * 0.5f;
    j->combustivel = RR_TANQUE_MAX;
    j->fase = RR_FASE_JOGANDO;
    j->invuln = RR_INVULN_FRAMES;
    for (int m = 0; m < RR_MAX_MISSEIS; m++) j->misseis[m].ativo = 0;
    memset(j->fx, 0, sizeof j->fx);
}

void rr_jogo_iniciar(rr_jogo *j, uint16_t seed, bool diario) {
    memset(j, 0, sizeof *j);
    j->seed = seed ? seed : (uint16_t)RR_SEED_CLASSICO;
    j->diario = diario ? 1 : 0;
    j->lfsr = j->lfsr_secao = j->seed;
    j->secao = 1;
    j->vidas = RR_VIDAS_INICIAIS;
    j->proxima_extra = RR_VIDA_EXTRA;
    j->acel = 1.0f;
    // seção "anterior" fictícia: reta contínua com o início do jogo
    for (int i = 0; i <= RR_TILES_POR_SECAO; i++)
        j->bordas_ant[i] = (rr_borda){ 80.0f, 52.0f };
    memset(j->tiles_ant, 0, sizeof j->tiles_ant);
    j->bordas[0] = (rr_borda){ 80.0f, 52.0f };
    entrar_na_secao(j);
    j->invuln = 0;   // sem piscar na primeira entrada
}

static void morrer(rr_jogo *j) {
    if (j->fase != RR_FASE_JOGANDO) return;
    j->fase = RR_FASE_EXPLODINDO;
    j->explosao_t = RR_EXPLOSAO_FRAMES;
    j->ev_explosao = 1;
    j->reabastecendo = 0;
    j->alarme = 0;
}

static void atualizar_entidades(rr_jogo *j) {
    float topo_tela = j->rolagem + (float)RR_ALTURA_JOGO;
    for (int k = 0; k < RR_MAX_ENTS; k++) {
        rr_ent *e = &j->ents[k];
        if (e->tipo == RR_ENT_LIVRE) continue;
        if (e->y < j->rolagem - 24.0f) { e->tipo = RR_ENT_LIVRE; continue; }  // saiu por baixo
        if (!e->ativo) {
            if (e->y < topo_tela + 40.0f) e->ativo = 1;   // ativação por proximidade (§7.5)
            else continue;
        }
        if (!e->vivo) continue;
        float esq, dir, ie, id;
        switch ((rr_ent_tipo)e->tipo) {
        case RR_ENT_NAVIO:
            e->x += e->vx;
            rr_mundo_margens(j, e->y, &esq, &dir, &ie, &id);
            if (e->x - 8.0f < esq + 1.0f) { e->x = esq + 9.0f; e->vx = -e->vx; }
            if (e->x + 8.0f > dir - 1.0f) { e->x = dir - 9.0f; e->vx = -e->vx; }
            break;
        case RR_ENT_HELI:
            // jitter + deslocamentos laterais súbitos, via PRNG da entidade
            if ((j->frame & 31u) == 0) {
                rr_lfsr_prox(&e->rnd);
                int r = e->rnd % 4;
                e->vx = (r == 0) ? 0.8f : (r == 1) ? -0.8f : 0.0f;
            }
            e->x += e->vx * 0.6f + (((e->rnd >> 3) & 1u) ? 0.15f : -0.15f);
            rr_mundo_margens(j, e->y, &esq, &dir, &ie, &id);
            e->x = clampf(e->x, esq + 7.0f, dir - 7.0f);
            break;
        case RR_ENT_JATO:
            e->x += e->vx;
            if (e->x < -16.0f || e->x > 176.0f) e->tipo = RR_ENT_LIVRE;
            break;
        default: break;   // depósito e ponte são estáticos
        }
    }
}

static void atualizar_misseis(rr_jogo *j, float vel_rolagem) {
    for (int m = 0; m < RR_MAX_MISSEIS; m++) {
        if (!j->misseis[m].ativo) continue;
        j->misseis[m].y += j->misseis[m].vy;
        if (j->misseis[m].y > j->rolagem + RR_ALTURA_JOGO + 8.0f) {
            j->misseis[m].ativo = 0;
            continue;
        }
        // acertos
        for (int k = 0; k < RR_MAX_ENTS; k++) {
            rr_ent *e = &j->ents[k];
            if (e->tipo == RR_ENT_LIVRE || !e->vivo || !e->ativo) continue;
            float mx, my; ent_dim(j, e, &mx, &my);
            if (aabb(j->misseis[m].x, j->misseis[m].y, 1.0f, 2.0f, e->x, e->y, mx, my)) {
                rr_ent_tipo tipo = (rr_ent_tipo)e->tipo;
                e->vivo = 0;
                if (tipo != RR_ENT_PONTE) e->tipo = RR_ENT_LIVRE;  // ponte destruída fica visível
                pontuar(j, pontos_ent(tipo));
                j->misseis[m].ativo = 0;
                j->ev_explosao = 1;
                adicionar_fx(j, e->x, e->y);
                break;
            }
        }
    }
    (void)vel_rolagem;
}

void rr_jogo_quadro(rr_jogo *j, const rr_entrada *e) {
    j->frame++;
    for (int i = 0; i < 8; i++) if (j->fx[i].t) j->fx[i].t--;

    if (j->fase == RR_FASE_FIM) return;

    if (j->fase == RR_FASE_EXPLODINDO) {
        if (--j->explosao_t <= 0) {
            j->vidas--;
            if (j->vidas <= 0) { j->vidas = 0; j->fase = RR_FASE_FIM; }
            else entrar_na_secao(j);
        }
        return;
    }

    // ---- acelerador no manche (mecânica-assinatura, §7.3): resposta imediata
    float alvo = e->eixo_y >= 0.0f
        ? 1.0f + e->eixo_y * (RR_ACEL_MAX - 1.0f)
        : 1.0f + e->eixo_y * (1.0f - RR_ACEL_MIN);
    j->acel = clampf(alvo, RR_ACEL_MIN, RR_ACEL_MAX);
    float vel = RR_VEL_BASE * j->acel;
    j->rolagem += vel;

    // ---- lateral sem inércia
    j->jato_x = clampf(j->jato_x + e->eixo_x * RR_VEL_LATERAL, 4.0f, 156.0f);

    // ---- tiro: cadência fixa, míssil herda a velocidade do jato
    if (j->cadencia > 0) j->cadencia--;
    if (e->fogo && j->cadencia == 0) {
        for (int m = 0; m < RR_MAX_MISSEIS; m++) {
            if (j->misseis[m].ativo) continue;
            j->misseis[m].ativo = 1;
            j->misseis[m].x = j->jato_x;
            j->misseis[m].y = j->rolagem + RR_JATO_MUNDO_DY + RR_JATO_ALT * 0.5f;
            j->misseis[m].vy = RR_MISSIL_VEL + vel;
            j->cadencia = RR_CADENCIA;
            j->ev_tiro = 1;
            break;
        }
    }

    atualizar_entidades(j);
    atualizar_misseis(j, vel);

    // ---- combustível (§7.4): queima constante, independente do acelerador
    float w_jato = j->rolagem + RR_JATO_MUNDO_DY;
    j->combustivel -= RR_QUEIMA;
    j->reabastecendo = 0;
    for (int k = 0; k < RR_MAX_ENTS; k++) {
        rr_ent *en = &j->ents[k];
        if (en->tipo != RR_ENT_DEPOSITO || !en->vivo || !en->ativo) continue;
        float mx, my; ent_dim(j, en, &mx, &my);
        if (aabb(j->jato_x, w_jato, RR_JATO_LARG * 0.5f, RR_JATO_ALT * 0.5f,
                 en->x, en->y, mx, my)) {
            j->combustivel += RR_RECARGA;
            j->reabastecendo = 1;
        }
    }
    j->combustivel = clampf(j->combustivel, 0.0f, RR_TANQUE_MAX);
    j->alarme = (j->combustivel < RR_ALARME_LIMIAR && !j->reabastecendo) ? 1 : 0;
    if (j->combustivel <= 0.0f) { morrer(j); return; }   // invulnerabilidade não salva do tanque vazio

    if (j->invuln > 0) j->invuln--;

    // ---- colisões do jato (o perigo é colisão, inimigos não atiram — §7.5)
    if (j->invuln == 0) {
        // margens e ilhas, linha a linha do sprite
        for (int r = 0; r < RR_JATO_ALT; r++) {
            float w = w_jato - RR_JATO_ALT * 0.5f + (float)r;
            float esq, dir, ie, id;
            rr_mundo_margens(j, w, &esq, &dir, &ie, &id);
            float jx0 = j->jato_x - RR_JATO_LARG * 0.5f;
            float jx1 = j->jato_x + RR_JATO_LARG * 0.5f;
            if (jx0 < esq || jx1 > dir) { morrer(j); return; }
            if (id > ie && jx1 > ie && jx0 < id) { morrer(j); return; }
        }
        // entidades (depósito é seguro de sobrevoar; ponte intacta mata)
        for (int k = 0; k < RR_MAX_ENTS; k++) {
            rr_ent *en = &j->ents[k];
            if (en->tipo == RR_ENT_LIVRE || !en->ativo) continue;
            if (en->tipo == RR_ENT_DEPOSITO) continue;
            if (en->tipo == RR_ENT_PONTE && !en->vivo) continue;
            if (!en->vivo) continue;
            float mx, my; ent_dim(j, en, &mx, &my);
            if (aabb(j->jato_x, w_jato, RR_JATO_LARG * 0.5f, RR_JATO_ALT * 0.5f,
                     en->x, en->y, mx, my)) { morrer(j); return; }
        }
    }

    // ---- avanço de seção: quando a base da tela cruza o fim da seção corrente
    float fim_secao = j->rolagem_secao + (float)(RR_TILES_POR_SECAO * RR_TILE_ALTURA);
    if (j->rolagem >= fim_secao) {
        memcpy(j->tiles_ant, j->tiles, sizeof j->tiles);
        memcpy(j->bordas_ant, j->bordas, sizeof j->bordas);
        j->bordas[0] = j->bordas[RR_TILES_POR_SECAO];
        j->rolagem_secao = fim_secao;
        j->secao++;
        j->lfsr_secao = j->lfsr;
        rr_mundo_gerar_secao(j);
    }
}

// ---------------------------------------------------------------- save state
#define RR_ESTADO_MAGIA 0x31535252u   // "RRS1" little-endian
#define RR_ESTADO_VERSAO 1u

typedef struct { uint32_t magia, versao, tamanho; } rr_estado_cab;

size_t rr_estado_tamanho(void) { return sizeof(rr_estado_cab) + sizeof(rr_jogo); }

size_t rr_estado_salvar(const rr_jogo *j, void *dst, size_t cap) {
    size_t total = rr_estado_tamanho();
    if (cap < total) return 0;
    rr_estado_cab cab = { RR_ESTADO_MAGIA, RR_ESTADO_VERSAO, (uint32_t)sizeof(rr_jogo) };
    memcpy(dst, &cab, sizeof cab);
    memcpy((uint8_t *)dst + sizeof cab, j, sizeof *j);
    return total;
}

bool rr_estado_carregar(rr_jogo *j, const void *src, size_t len) {
    if (len < sizeof(rr_estado_cab)) return false;
    rr_estado_cab cab;
    memcpy(&cab, src, sizeof cab);
    if (cab.magia != RR_ESTADO_MAGIA || cab.versao != RR_ESTADO_VERSAO) return false;
    if (cab.tamanho != sizeof(rr_jogo) || len < sizeof cab + sizeof(rr_jogo)) return false;
    memcpy(j, (const uint8_t *)src + sizeof cab, sizeof *j);
    return true;
}

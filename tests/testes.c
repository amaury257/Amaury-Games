// testes.c — Testes obrigatórios do §7.13/§12, executáveis em qualquer host
// (o motor é C puro). Rodar com scripts/rodar-testes.sh.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../Sources/Game/engine/rr_jogo.h"
#include "../Sources/Game/engine/rr_render.h"
#include "../Sources/Game/engine/rr_recordes.h"

static int falhas = 0;
#define VERIFICA(cond, msg) do { \
    if (cond) printf("  ok  %s\n", msg); \
    else { printf("  FALHA  %s (linha %d)\n", msg, __LINE__); falhas++; } \
} while (0)

// ---------------------------------------------------------------- golden LFSR (D6)
// Sequência pinada: primeiras 8 saídas do LFSR taps [16,14,13,11], seed 0xACE1.
// Gerada uma única vez por este próprio código e congelada — qualquer mudança
// no LFSR quebra este teste de propósito.
static void teste_lfsr_golden(void) {
    printf("LFSR golden:\n");
    static const uint16_t ESPERADO[8] = {
        0x5670, 0xAB38, 0x559C, 0x2ACE, 0x1567, 0x8AB3, 0x4559, 0x22AC
    };
    uint16_t l = RR_SEED_CLASSICO;
    int ok = 1;
    for (int i = 0; i < 8; i++) {
        uint16_t v = rr_lfsr_prox(&l);
        if (v != ESPERADO[i]) {
            printf("    posicao %d: obtido %04X, esperado %04X\n", i, v, ESPERADO[i]);
            ok = 0;
        }
    }
    VERIFICA(ok, "seed 0xACE1 produz a sequencia pinada");
    // sanidade de período: não pode voltar à seed cedo demais
    l = RR_SEED_CLASSICO;
    int voltas = 0;
    for (int i = 0; i < 65535; i++) if (rr_lfsr_prox(&l) == RR_SEED_CLASSICO) voltas++;
    VERIFICA(voltas == 1, "periodo maximo (seed reaparece 1 vez em 65535 passos)");
}

// ---------------------------------------------------------------- golden do mapa (§7.2)
// Mesma seed ⇒ sequência de peças byte-idêntica.
static void teste_mapa_golden(void) {
    printf("Mapa golden (secao 1, seed 0xACE1):\n");
    static const uint8_t ESPERADO[RR_TILES_POR_SECAO] = {
        0, 3, 2, 6, 0, 4, 1, 0, 4, 1, 6, 2, 2, 4, 2, 4, 0, 6, 2, 7
    };
    rr_jogo j;
    rr_jogo_iniciar(&j, RR_SEED_CLASSICO, false);
    uint8_t tipos[RR_TILES_POR_SECAO];
    for (int i = 0; i < RR_TILES_POR_SECAO; i++) tipos[i] = j.tiles[i].tipo;
    if (memcmp(tipos, ESPERADO, sizeof tipos) != 0) {
        printf("    obtido: {");
        for (int i = 0; i < RR_TILES_POR_SECAO; i++) printf("%d,", tipos[i]);
        printf("}\n");
    }
    VERIFICA(memcmp(tipos, ESPERADO, sizeof tipos) == 0, "pecas da secao 1 byte-identicas");
    VERIFICA(tipos[RR_TILES_POR_SECAO - 1] == RR_TILE_PONTE, "toda secao termina em ponte");

    // dois jogos, mesma seed e mesmas entradas ⇒ mesmo framebuffer
    rr_jogo a, b;
    rr_jogo_iniciar(&a, 0x1234, false);
    rr_jogo_iniciar(&b, 0x1234, false);
    rr_entrada e = { 0.5f, 0.3f, true };
    for (int f = 0; f < 300; f++) { rr_jogo_quadro(&a, &e); rr_jogo_quadro(&b, &e); }
    uint32_t *fa = malloc(RR_LARGURA * RR_ALTURA * 4), *fb = malloc(RR_LARGURA * RR_ALTURA * 4);
    rr_render_quadro(&a, fa); rr_render_quadro(&b, fb);
    VERIFICA(rr_render_hash(fa) == rr_render_hash(fb), "mesma seed+entradas => framebuffer identico");
    free(fa); free(fb);
}

// ---------------------------------------------------------------- combustível (§7.4)
static void teste_combustivel(void) {
    printf("Combustivel:\n");
    rr_jogo j;
    rr_jogo_iniciar(&j, RR_SEED_CLASSICO, false);
    float antes = j.combustivel;
    // queima determinística: N frames * RR_QUEIMA (sem depósito por perto no frame 1)
    rr_entrada parado = { 0 };
    rr_jogo_quadro(&j, &parado);
    float delta = antes - j.combustivel;
    VERIFICA(delta > RR_QUEIMA * 0.99f && delta < RR_QUEIMA * 1.01f,
             "queima por frame igual a RR_QUEIMA");
    // tanque cheio ≈ 60 s em cruzeiro
    float frames_ate_zero = RR_TANQUE_MAX / RR_QUEIMA;
    VERIFICA(frames_ate_zero >= 60.0f * RR_FPS * 0.99f && frames_ate_zero <= 60.0f * RR_FPS * 1.01f,
             "tanque cheio dura ~60 s");
    // recarga: cheio em ~4 s sobre o depósito (recarga - queima líquida)
    float liquida = RR_RECARGA - RR_QUEIMA;
    float frames_recarga = RR_TANQUE_MAX / liquida;
    VERIFICA(frames_recarga < 4.4f * RR_FPS, "recarga enche o tanque em ~4 s");
    // zerar combustível mata mesmo invulnerável
    j.combustivel = RR_QUEIMA * 0.5f;
    j.invuln = 999;
    rr_jogo_quadro(&j, &parado);
    VERIFICA(j.fase == RR_FASE_EXPLODINDO, "combustivel zerado perde vida (mesmo invulneravel)");
}

// ---------------------------------------------------------------- colisão AABB (§7.13)
// Geometria aplainada (rio reto 80±52, sem ilhas nem inimigos) para isolar os
// casos de borda: encostar NÃO colide; sobrepor colide.
static void aplainar(rr_jogo *j) {
    for (int i = 0; i <= RR_TILES_POR_SECAO; i++) {
        j->bordas[i] = (rr_borda){ 80.0f, 52.0f };
        j->bordas_ant[i] = (rr_borda){ 80.0f, 52.0f };
    }
    for (int i = 0; i < RR_TILES_POR_SECAO; i++) {
        j->tiles[i] = (rr_tile){ RR_TILE_RETO_LARGO, 0.0f };
        j->tiles_ant[i] = (rr_tile){ RR_TILE_RETO_LARGO, 0.0f };
    }
    for (int k = 0; k < RR_MAX_ENTS; k++) j->ents[k].tipo = RR_ENT_LIVRE;
}

static void teste_colisao(void) {
    printf("Colisao AABB (casos de borda):\n");
    rr_jogo j;
    rr_entrada parado = { 0 };
    const float esq = 80.0f - 52.0f;
    const float dir = 80.0f + 52.0f;
    const float meia_jato = RR_JATO_LARG * 0.5f;

    // exatamente encostado na margem esquerda (aresta compartilhada): vivo
    rr_jogo_iniciar(&j, RR_SEED_CLASSICO, false);
    aplainar(&j);
    j.jato_x = esq + meia_jato;
    rr_jogo_quadro(&j, &parado);
    VERIFICA(j.fase == RR_FASE_JOGANDO, "aresta exata na margem esquerda: vivo");

    // sobrepondo a margem esquerda: morre
    rr_jogo_iniciar(&j, RR_SEED_CLASSICO, false);
    aplainar(&j);
    j.jato_x = esq + meia_jato - 1.5f;
    rr_jogo_quadro(&j, &parado);
    VERIFICA(j.fase == RR_FASE_EXPLODINDO, "sobrepondo a margem esquerda: morre");

    // sobrepondo a margem direita: morre
    rr_jogo_iniciar(&j, RR_SEED_CLASSICO, false);
    aplainar(&j);
    j.jato_x = dir - meia_jato + 1.5f;
    rr_jogo_quadro(&j, &parado);
    VERIFICA(j.fase == RR_FASE_EXPLODINDO, "sobrepondo a margem direita: morre");

    // entidade exatamente encostada (dx = soma das meias-larguras): vivo
    rr_jogo_iniciar(&j, RR_SEED_CLASSICO, false);
    aplainar(&j);
    j.jato_x = 80.0f;
    float y_jato = j.rolagem + RR_VEL_BASE + RR_JATO_MUNDO_DY;  // posição no fim do quadro
    j.ents[0] = (rr_ent){ RR_ENT_NAVIO, 1, 1, 80.0f + meia_jato + 8.0f, y_jato, 0.0f, 1 };
    rr_jogo_quadro(&j, &parado);
    VERIFICA(j.fase == RR_FASE_JOGANDO, "navio encostado sem sobrepor: vivo");

    // entidade sobreposta em 1 px: morre
    rr_jogo_iniciar(&j, RR_SEED_CLASSICO, false);
    aplainar(&j);
    j.jato_x = 80.0f;
    y_jato = j.rolagem + RR_VEL_BASE + RR_JATO_MUNDO_DY;
    j.ents[0] = (rr_ent){ RR_ENT_NAVIO, 1, 1, 80.0f + meia_jato + 7.0f, y_jato, 0.0f, 1 };
    rr_jogo_quadro(&j, &parado);
    VERIFICA(j.fase == RR_FASE_EXPLODINDO, "navio sobreposto em 1 px: morre");

    // ilha: sobrepor a ilha mata
    rr_jogo_iniciar(&j, RR_SEED_CLASSICO, false);
    aplainar(&j);
    for (int i = 0; i < RR_TILES_POR_SECAO; i++) j.tiles[i] = (rr_tile){ RR_TILE_ILHA, 12.0f };
    j.jato_x = 80.0f;   // centro do canal = centro da ilha
    rr_jogo_quadro(&j, &parado);
    VERIFICA(j.fase == RR_FASE_EXPLODINDO, "sobrepondo a ilha: morre");
}

// ---------------------------------------------------------------- pontuação e vida extra (§7.7)
static void teste_vida_extra(void) {
    printf("Vida extra em 10.000:\n");
    rr_jogo j;
    rr_jogo_iniciar(&j, RR_SEED_CLASSICO, false);
    int32_t vidas0 = j.vidas;

    // simula a pontuação cruzando 10.000 via caminho oficial (míssil acerta ponte)
    // fabricando o cenário: entidade ponte + míssil em cima dela
    j.pontos = 9990;
    j.proxima_extra = RR_VIDA_EXTRA;
    // usa um depósito fabricado (80 pts) longe do jato
    for (int k = 0; k < RR_MAX_ENTS; k++) j.ents[k].tipo = RR_ENT_LIVRE;
    j.ents[0] = (rr_ent){ RR_ENT_DEPOSITO, 1, 1, 80.0f, j.rolagem + 120.0f, 0.0f, 1 };
    j.misseis[0].ativo = 1;
    j.misseis[0].x = 80.0f;
    j.misseis[0].y = j.rolagem + 118.0f;
    j.misseis[0].vy = 4.0f;
    rr_entrada parado = { 0 };
    rr_jogo_quadro(&j, &parado);
    VERIFICA(j.pontos == 9990 + RR_PTS_DEPOSITO, "deposito atingido vale 80 pontos");
    VERIFICA(j.vidas == vidas0 + 1, "cruzar 10.000 concede vida extra");
    VERIFICA(j.proxima_extra == 2 * RR_VIDA_EXTRA, "proxima vida extra em 20.000");
    VERIFICA(j.ev_vida_extra == 1, "evento de jingle sinalizado");
}

// ---------------------------------------------------------------- save state (§12)
static void teste_save_state(void) {
    printf("Save state round-trip:\n");
    rr_jogo j;
    rr_jogo_iniciar(&j, RR_SEED_CLASSICO, false);
    rr_entrada e = { 0.3f, 0.6f, true };
    for (int f = 0; f < 240; f++) rr_jogo_quadro(&j, &e);

    uint32_t *fb1 = malloc(RR_LARGURA * RR_ALTURA * 4);
    rr_render_quadro(&j, fb1);
    uint32_t hash_antes = rr_render_hash(fb1);

    size_t cap = rr_estado_tamanho();
    void *blob = malloc(cap);
    VERIFICA(rr_estado_salvar(&j, blob, cap) == cap, "serializa no tamanho esperado");

    // avança o jogo (diverge), depois restaura
    for (int f = 0; f < 120; f++) rr_jogo_quadro(&j, &e);
    rr_render_quadro(&j, fb1);
    VERIFICA(rr_render_hash(fb1) != hash_antes, "estado divergiu apos avancar");

    rr_jogo j2;
    VERIFICA(rr_estado_carregar(&j2, blob, cap), "unserialize valido");
    rr_render_quadro(&j2, fb1);
    VERIFICA(rr_render_hash(fb1) == hash_antes, "hash do framebuffer identico apos restaurar");

    // blob corrompido é rejeitado
    ((uint8_t *)blob)[0] ^= 0xFF;
    VERIFICA(!rr_estado_carregar(&j2, blob, cap), "magia invalida rejeitada");

    free(blob); free(fb1);
}

// ---------------------------------------------------------------- recordes (§7.13)
static void teste_recordes(void) {
    printf("Recordes round-trip:\n");
    rr_recordes t;
    rr_recordes_zerar(&t);
    VERIFICA(rr_recordes_inserir(&t, "AMA", 5000) == 0, "primeiro recorde entra em 1o");
    VERIFICA(rr_recordes_inserir(&t, "URY", 9000) == 0, "maior pontuacao assume o topo");
    VERIFICA(rr_recordes_inserir(&t, "ZZZ", 7000) == 1, "intermediario ordenado");
    for (int i = 0; i < RR_RECORDES_N; i++) rr_recordes_inserir(&t, "ENC", 10000 + i);
    VERIFICA(!rr_recordes_entra(&t, 1), "pontuacao baixa nao entra na tabela cheia");

    uint8_t blob[512];
    size_t n = rr_recordes_salvar(&t, blob, sizeof blob);
    VERIFICA(n == rr_recordes_tamanho(), "serializa no tamanho esperado");
    rr_recordes t2;
    VERIFICA(rr_recordes_carregar(&t2, blob, n), "carrega de volta");
    VERIFICA(memcmp(&t, &t2, sizeof t) == 0, "round-trip byte-identico");
}

// ---------------------------------------------------------------- seed do Diário (§7.11)
static void teste_seed_diario(void) {
    printf("Seed do modo Diario:\n");
    VERIFICA(rr_seed_diario(20260712) == rr_seed_diario(20260712), "mesma data => mesma seed");
    VERIFICA(rr_seed_diario(20260712) != rr_seed_diario(20260713), "datas diferentes => seeds diferentes");
    VERIFICA(rr_seed_diario(0) != 0, "seed nunca e zero (LFSR nao trava)");
}

int main(void) {
    printf("== Testes do motor River Raid ==\n");
    teste_lfsr_golden();
    teste_mapa_golden();
    teste_combustivel();
    teste_colisao();
    teste_vida_extra();
    teste_save_state();
    teste_recordes();
    teste_seed_diario();
    if (falhas) { printf("\n%d FALHA(S)\n", falhas); return 1; }
    printf("\nTodos os testes passaram.\n");
    return 0;
}

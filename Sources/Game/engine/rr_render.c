// rr_render.c — Renderizador por software (paleta reduzida ~10 cores, §7.1).
// Sprites 100% próprios, formas mínimas evocando o traço do 2600.
#include "rr_render.h"
#include <string.h>

// ---------------------------------------------------------------- paleta
#define C_AGUA      0xFF2038C8u
#define C_MARGEM    0xFF2C8830u
#define C_MARGEM_2  0xFF247024u
#define C_HUD       0xFF303038u
#define C_PRETO     0xFF101014u
#define C_BRANCO    0xFFF0F0F0u
#define C_AMARELO   0xFFE8D850u
#define C_CINZA     0xFF9098A0u
#define C_LARANJA   0xFFE07820u
#define C_VERMELHO  0xFFD03028u
#define C_PONTE     0xFF8A5A2Cu

// ---------------------------------------------------------------- fonte 3×5 própria
// 0-9 A-Z; 3 bits por linha, 5 linhas por glifo.
static const uint8_t FONTE[36][5] = {
    {7,5,5,5,7},{2,6,2,2,7},{7,1,7,4,7},{7,1,7,1,7},{5,5,7,1,1},
    {7,4,7,1,7},{7,4,7,5,7},{7,1,1,2,2},{7,5,7,5,7},{7,5,7,1,7},
    {2,5,7,5,5},{6,5,6,5,6},{3,4,4,4,3},{6,5,5,5,6},{7,4,6,4,7},
    {7,4,6,4,4},{3,4,5,5,3},{5,5,7,5,5},{7,2,2,2,7},{1,1,1,5,2},
    {5,6,4,6,5},{4,4,4,4,7},{5,7,7,5,5},{6,5,5,5,5},{2,5,5,5,2},
    {6,5,6,4,4},{2,5,5,6,3},{6,5,6,6,5},{3,4,2,1,6},{7,2,2,2,2},
    {5,5,5,5,7},{5,5,5,5,2},{5,5,7,7,5},{5,5,2,5,5},{5,5,2,2,2},
    {7,1,2,4,7},
};

static int indice_glifo(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'Z') return 10 + (c - 'A');
    return -1;
}

static void pixel(uint32_t *fb, int x, int y, uint32_t cor) {
    if (x >= 0 && x < RR_LARGURA && y >= 0 && y < RR_ALTURA)
        fb[y * RR_LARGURA + x] = cor;
}

static void retangulo(uint32_t *fb, int x, int y, int w, int h, uint32_t cor) {
    for (int r = 0; r < h; r++)
        for (int c = 0; c < w; c++)
            pixel(fb, x + c, y + r, cor);
}

static void texto(uint32_t *fb, int x, int y, const char *s, uint32_t cor, int esc) {
    for (; *s; s++, x += 4 * esc) {
        int g = indice_glifo(*s);
        if (g < 0) continue;
        for (int r = 0; r < 5; r++)
            for (int c = 0; c < 3; c++)
                if (FONTE[g][r] & (4 >> c))
                    retangulo(fb, x + c * esc, y + r * esc, esc, esc, cor);
    }
}

// ---------------------------------------------------------------- sprites próprios
static const uint8_t SPR_JATO[10] = {
    0x18,0x18,0x3C,0x3C,0x7E,0xFF,0xFF,0x66,0x24,0x24 };
static const uint16_t SPR_NAVIO[6] = {
    0x0380,0x07C0,0x7FFE,0xFFFF,0xFFFF,0x7FFE };
static const uint16_t SPR_HELI[8] = {
    0x0FFF,0x00F0,0x01F8,0x07FE,0x0FFF,0x07FE,0x00F0,0x0020 };
static const uint16_t SPR_JATO_INIMIGO[6] = {
    0x0C00,0x1E00,0xFF00,0xFFC0,0x1E00,0x0C00 };

static void sprite8(uint32_t *fb, int x, int y, const uint8_t *spr, int alt, uint32_t cor) {
    for (int r = 0; r < alt; r++)
        for (int c = 0; c < 8; c++)
            if (spr[r] & (0x80u >> c)) pixel(fb, x + c, y + r, cor);
}

static void sprite16(uint32_t *fb, int x, int y, const uint16_t *spr, int alt, int larg, uint32_t cor) {
    for (int r = 0; r < alt; r++)
        for (int c = 0; c < larg; c++)
            if (spr[r] & (1u << (larg - 1 - c))) pixel(fb, x + c, y + r, cor);
}

// explosão procedural de 4 quadros: blocos que se expandem (§7.8)
static void explosao(uint32_t *fb, int cx, int cy, int t /*24..0*/) {
    static const int8_t DX[8] = { -1, 1, -1, 1, 0, -2, 2, 0 };
    static const int8_t DY[8] = { -1, -1, 1, 1, -2, 0, 0, 2 };
    int quadro = 3 - (t - 1) / 6;      // 0..3
    int raio = 1 + quadro * 2;
    uint32_t cor = (quadro < 2) ? C_AMARELO : C_LARANJA;
    for (int i = 0; i < 8; i++)
        retangulo(fb, cx + DX[i] * raio - 1, cy + DY[i] * raio - 1, 2, 2, cor);
    if (quadro == 0) retangulo(fb, cx - 2, cy - 2, 4, 4, C_BRANCO);
}

// linha de tela r (0..RR_ALTURA_JOGO-1) → linha de mundo
static float mundo_y(const rr_jogo *j, int r) {
    return j->rolagem + (float)(RR_ALTURA_JOGO - 1 - r);
}
static int tela_y(const rr_jogo *j, float y) {
    return RR_ALTURA_JOGO - 1 - (int)(y - j->rolagem + 0.5f);
}

// ---------------------------------------------------------------- HUD (§7.10)
static void hud(const rr_jogo *j, uint32_t *fb) {
    retangulo(fb, 0, RR_ALTURA_JOGO, RR_LARGURA, RR_ALTURA_HUD, C_HUD);
    retangulo(fb, 0, RR_ALTURA_JOGO, RR_LARGURA, 1, C_PRETO);
    int y0 = RR_ALTURA_JOGO + 4;

    // pontuação em fonte digital própria (6 dígitos, escala 2)
    char pts[8];
    uint32_t p = j->pontos > 999999u ? 999999u : j->pontos;
    for (int i = 5; i >= 0; i--) { pts[i] = (char)('0' + p % 10u); p /= 10u; }
    pts[6] = 0;
    texto(fb, 4, y0, pts, C_AMARELO, 2);

    // seção (direita)
    char sec[4];
    uint32_t s = j->secao > 99 ? 99 : j->secao;
    sec[0] = (char)('0' + s / 10u); sec[1] = (char)('0' + s % 10u); sec[2] = 0;
    texto(fb, 146, y0, sec, C_CINZA, 1);

    // medidor de combustível analógico: E · ½ · F com agulha (§7.10)
    int gx = 52, gy = RR_ALTURA_JOGO + 17, gw = 76, gh = 10;
    retangulo(fb, gx - 1, gy - 1, gw + 2, gh + 2, C_CINZA);
    retangulo(fb, gx, gy, gw, gh, C_PRETO);
    retangulo(fb, gx, gy, (int)((float)gw * RR_ALARME_LIMIAR / RR_TANQUE_MAX), gh, 0xFF501418u);
    texto(fb, gx - 8, gy + 2, "E", C_BRANCO, 1);
    texto(fb, gx + gw + 4, gy + 2, "F", C_BRANCO, 1);
    pixel(fb, gx + gw / 2, gy - 2, C_BRANCO);            // marca ½
    pixel(fb, gx + gw / 2, gy + gh + 1, C_BRANCO);
    int agulha = gx + (int)((float)(gw - 1) * j->combustivel / RR_TANQUE_MAX);
    uint32_t cor_agulha = j->alarme && ((j->frame >> 3) & 1u) ? C_VERMELHO : C_AMARELO;
    retangulo(fb, agulha, gy, 2, gh, cor_agulha);

    // vidas: ícones mínimos do jato
    for (int v = 0; v < (j->vidas > 6 ? 6 : j->vidas); v++) {
        int lx = 6 + v * 8, ly = RR_ALTURA_JOGO + 20;
        pixel(fb, lx + 2, ly, C_BRANCO);
        retangulo(fb, lx + 1, ly + 1, 3, 1, C_BRANCO);
        retangulo(fb, lx, ly + 2, 5, 1, C_BRANCO);
        pixel(fb, lx + 2, ly + 3, C_BRANCO);
    }
}

// ---------------------------------------------------------------- quadro completo
void rr_render_quadro(const rr_jogo *j, uint32_t *fb) {
    // cenário: margens, canal e ilhas, linha a linha
    for (int r = 0; r < RR_ALTURA_JOGO; r++) {
        float y = mundo_y(j, r);
        float esq, dir, ie, id;
        rr_mundo_margens(j, y, &esq, &dir, &ie, &id);
        int e = (int)esq, d = (int)dir;
        // faixas sutis na vegetação, presas ao mundo (rolam junto)
        uint32_t verde = (((int)y >> 3) & 1) ? C_MARGEM : C_MARGEM_2;
        uint32_t *linha = fb + r * RR_LARGURA;
        for (int x = 0; x < RR_LARGURA; x++)
            linha[x] = (x < e || x > d) ? verde : C_AGUA;
        if (id > ie) {
            int a = (int)ie, b = (int)id;
            for (int x = a; x <= b; x++)
                if (x >= 0 && x < RR_LARGURA) linha[x] = verde;
        }
    }

    // entidades
    for (int k = 0; k < RR_MAX_ENTS; k++) {
        const rr_ent *e = &j->ents[k];
        if (e->tipo == RR_ENT_LIVRE) continue;
        int sy = tela_y(j, e->y);
        if (sy < -20 || sy > RR_ALTURA_JOGO + 20) continue;
        int sx = (int)e->x;
        switch ((rr_ent_tipo)e->tipo) {
        case RR_ENT_NAVIO:
            if (e->vivo) sprite16(fb, sx - 8, sy - 3, SPR_NAVIO, 6, 16, C_CINZA);
            break;
        case RR_ENT_HELI:
            if (e->vivo) {
                sprite16(fb, sx - 6, sy - 4, SPR_HELI, 8, 12, C_LARANJA);
                // rotor girando
                if ((j->frame >> 2) & 1u) retangulo(fb, sx - 6, sy - 4, 12, 1, C_BRANCO);
            }
            break;
        case RR_ENT_JATO:
            if (e->vivo) sprite16(fb, sx - 5, sy - 3, SPR_JATO_INIMIGO, 6, 10, C_BRANCO);
            break;
        case RR_ENT_DEPOSITO:
            if (e->vivo) {
                retangulo(fb, sx - 7, sy - 8, 14, 16, C_CINZA);
                retangulo(fb, sx - 6, sy - 7, 12, 14, C_VERMELHO);
                texto(fb, sx - 5, sy - 2, "GAS", C_BRANCO, 1);
            }
            break;
        case RR_ENT_PONTE: {
            float esq, dir, ie, id;
            rr_mundo_margens(j, e->y, &esq, &dir, &ie, &id);
            if (e->vivo) {
                for (int r = -5; r < 5; r++)
                    for (int x = (int)esq; x <= (int)dir; x++) {
                        int py = sy + r;
                        // treliça: bordas cheias, miolo em xadrez
                        if (r <= -4 || r >= 3 || (((x + py) & 3) < 2))
                            pixel(fb, x, py, C_PONTE);
                    }
                texto(fb, (int)((esq + dir) * 0.5f) - 5, sy - 2, "RR", C_AMARELO, 1);
            } else {
                // escombros nas cabeceiras
                retangulo(fb, (int)esq, sy - 4, 6, 8, C_PONTE);
                retangulo(fb, (int)dir - 6, sy - 4, 6, 8, C_PONTE);
            }
            break;
        }
        default: break;
        }
    }

    // mísseis
    for (int m = 0; m < RR_MAX_MISSEIS; m++)
        if (j->misseis[m].ativo)
            retangulo(fb, (int)j->misseis[m].x - 1, tela_y(j, j->misseis[m].y) - 2, 2, 4, C_BRANCO);

    // explosões de alvos
    for (int i = 0; i < 8; i++)
        if (j->fx[i].t) explosao(fb, (int)j->fx[i].x, tela_y(j, j->fx[i].y), j->fx[i].t);

    // jato do jogador (pisca durante a invulnerabilidade pós-respawn)
    if (j->fase == RR_FASE_JOGANDO) {
        if (!(j->invuln > 0 && ((j->frame >> 2) & 1u)))
            sprite8(fb, (int)j->jato_x - 4, RR_JATO_Y, SPR_JATO, RR_JATO_ALT, C_AMARELO);
    } else if (j->fase == RR_FASE_EXPLODINDO) {
        explosao(fb, (int)j->jato_x, RR_JATO_Y + 5, j->explosao_t > 24 ? 24 : j->explosao_t);
    } else {  // FIM
        texto(fb, 32, 70, "FIM DE JOGO", C_BRANCO, 2);
    }

    hud(j, fb);
}

uint32_t rr_render_hash(const uint32_t *fb) {
    uint32_t h = 2166136261u;
    for (int i = 0; i < RR_LARGURA * RR_ALTURA; i++) {
        h ^= fb[i];
        h *= 16777619u;
    }
    return h;
}

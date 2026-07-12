#include "rr_recordes.h"
#include <string.h>

#define MAGIA 0x31525252u   // "RRR1"

void rr_recordes_zerar(rr_recordes *t) { memset(t, 0, sizeof *t); }

bool rr_recordes_entra(const rr_recordes *t, uint32_t pontos) {
    return pontos > 0 && pontos > t->itens[RR_RECORDES_N - 1].pontos;
}

int rr_recordes_inserir(rr_recordes *t, const char nome[3], uint32_t pontos) {
    if (!rr_recordes_entra(t, pontos)) return -1;
    int pos = RR_RECORDES_N - 1;
    while (pos > 0 && t->itens[pos - 1].pontos < pontos) {
        t->itens[pos] = t->itens[pos - 1];
        pos--;
    }
    memcpy(t->itens[pos].nome, nome, 3);
    t->itens[pos].nome[3] = 0;
    t->itens[pos].pontos = pontos;
    return pos;
}

size_t rr_recordes_tamanho(void) { return 4 + sizeof(rr_recordes); }

size_t rr_recordes_salvar(const rr_recordes *t, void *dst, size_t cap) {
    size_t total = rr_recordes_tamanho();
    if (cap < total) return 0;
    uint32_t magia = MAGIA;
    memcpy(dst, &magia, 4);
    memcpy((uint8_t *)dst + 4, t, sizeof *t);
    return total;
}

bool rr_recordes_carregar(rr_recordes *t, const void *src, size_t len) {
    if (len < rr_recordes_tamanho()) return false;
    uint32_t magia;
    memcpy(&magia, src, 4);
    if (magia != MAGIA) return false;
    memcpy(t, (const uint8_t *)src + 4, sizeof *t);
    return true;
}

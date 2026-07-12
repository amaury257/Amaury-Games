// rr_lfsr.h — LFSR Fibonacci de 16 bits, taps [16, 14, 13, 11] (D6).
// Período máximo (65535); mesma seed ⇒ mesma sequência, sempre.
#ifndef RR_LFSR_H
#define RR_LFSR_H
#include <stdint.h>

static inline uint16_t rr_lfsr_prox(uint16_t *estado) {
    uint16_t l = *estado;
    // taps 16,14,13,11 contados do bit mais significativo ⇒ bits 0,2,3,5 do registrador
    uint16_t bit = (uint16_t)(((l >> 0) ^ (l >> 2) ^ (l >> 3) ^ (l >> 5)) & 1u);
    l = (uint16_t)((l >> 1) | (bit << 15));
    *estado = l;
    return l;
}

#endif

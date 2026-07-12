// rr_render.h — Renderizador por software do River Raid.
// Desenha o quadro completo (jogo + HUD) num framebuffer 160×192 XRGB8888;
// a camada Metal só sobe a textura e escala por fator inteiro (D4).
#ifndef RR_RENDER_H
#define RR_RENDER_H
#include "rr_jogo.h"

void rr_render_quadro(const rr_jogo *j, uint32_t *fb);

// hash FNV-1a do framebuffer — usado no teste de round-trip de save state (§12)
uint32_t rr_render_hash(const uint32_t *fb);

#endif

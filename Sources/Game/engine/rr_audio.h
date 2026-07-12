// rr_audio.h — Sintetizador estilo TIA, 2 canais (§7.9). Sem samples, sem
// arquivos: onda quadrada (motor/tiro/recarga/jingle) + ruído (explosão/alarme).
// O estado é separado do rr_jogo (não entra no save state); a sessão copia os
// eventos do jogo para cá uma vez por quadro, sob o mesmo lock do pull de áudio.
#ifndef RR_AUDIO_H
#define RR_AUDIO_H
#include <stdint.h>
#include "rr_jogo.h"

typedef struct {
    // canal 1 — onda quadrada
    float fase_motor;
    float freq_motor;        // acoplada ao acelerador
    int   tiro_t;            // frames de blip restantes (em amostras)
    float fase_fx;
    int   recarga_amostras;  // fase do tom ascendente de reabastecimento
    int   jingle_amostras;   // jingle de vida extra (2 notas)
    uint8_t reabastecendo;
    // canal 2 — ruído
    uint32_t ruido;
    float env_explosao;
    uint8_t alarme;
    int   alarme_fase;
} rr_audio;

void rr_audio_iniciar(rr_audio *a);
// copia estados/eventos do jogo (chamar 1× por quadro, consumindo os ev_*)
void rr_audio_sincronizar(rr_audio *a, rr_jogo *j);
// gera n amostras mono int16 @ RR_TAXA_AUDIO (modelo pull)
void rr_audio_processar(rr_audio *a, int16_t *dst, int n);

#endif

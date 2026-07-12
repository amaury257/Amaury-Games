// rr_audio.c — Sintetizador estilo TIA (§7.9): 2 canais, zero samples.
// Canal 1 (quadrada): drone do motor (pitch ~ acelerador), blip de tiro,
// tom ascendente de reabastecimento, jingle de vida extra (2 notas).
// Canal 2 (ruído): explosão com decay, alarme de combustível.
#include "rr_audio.h"

#define TAXA ((float)RR_TAXA_AUDIO)

void rr_audio_iniciar(rr_audio *a) {
    *a = (rr_audio){0};
    a->ruido = 0x1D872B41u;
    a->freq_motor = 70.0f;
}

void rr_audio_sincronizar(rr_audio *a, rr_jogo *j) {
    a->freq_motor = 50.0f + j->acel * 45.0f;
    a->reabastecendo = j->reabastecendo;
    a->alarme = j->alarme;
    if (j->ev_tiro)       { a->tiro_t = (int)(TAXA * 0.07f); j->ev_tiro = 0; }
    if (j->ev_explosao)   { a->env_explosao = 1.0f;          j->ev_explosao = 0; }
    if (j->ev_vida_extra) { a->jingle_amostras = (int)(TAXA * 0.30f); j->ev_vida_extra = 0; }
    if (!a->reabastecendo) a->recarga_amostras = 0;
}

static inline float quadrada(float *fase, float freq) {
    *fase += freq / TAXA;
    if (*fase >= 1.0f) *fase -= 1.0f;
    return *fase < 0.5f ? 1.0f : -1.0f;
}

void rr_audio_processar(rr_audio *a, int16_t *dst, int n) {
    for (int i = 0; i < n; i++) {
        float ch1;
        // prioridade no canal 1: jingle > tiro > recarga > motor
        if (a->jingle_amostras > 0) {
            a->jingle_amostras--;
            float f = a->jingle_amostras > (int)(TAXA * 0.15f) ? 660.0f : 990.0f;
            ch1 = quadrada(&a->fase_fx, f) * 0.28f;
        } else if (a->tiro_t > 0) {
            a->tiro_t--;
            float f = 700.0f + 800.0f * (float)a->tiro_t / (TAXA * 0.07f);
            ch1 = quadrada(&a->fase_fx, f) * 0.30f;
        } else if (a->reabastecendo) {
            a->recarga_amostras++;
            if (a->recarga_amostras >= (int)(TAXA * 0.8f)) a->recarga_amostras = 0;
            float f = 200.0f + 700.0f * (float)a->recarga_amostras / (TAXA * 0.8f);
            ch1 = quadrada(&a->fase_fx, f) * 0.20f;
        } else {
            ch1 = quadrada(&a->fase_motor, a->freq_motor) * 0.12f;
        }

        // canal 2: ruído (xorshift) com envelope de explosão + alarme pulsado
        a->ruido ^= a->ruido << 13;
        a->ruido ^= a->ruido >> 17;
        a->ruido ^= a->ruido << 5;
        float ruido = ((float)(a->ruido & 0xFFFFu) / 32768.0f) - 1.0f;
        float ch2 = 0.0f;
        if (a->env_explosao > 0.001f) {
            ch2 += ruido * a->env_explosao * 0.35f;
            a->env_explosao *= 0.99988f;             // decay ≈ 0,5 s
        }
        if (a->alarme) {
            a->alarme_fase++;
            if (a->alarme_fase >= (int)(TAXA * 0.6f)) a->alarme_fase = 0;
            if (a->alarme_fase < (int)(TAXA * 0.08f))
                ch2 += (ruido * 0.5f + 0.5f) * 0.22f;  // bipe rugoso periódico
        } else if (a->alarme_fase) {
            a->alarme_fase = 0;
        }

        float mix = ch1 + ch2;
        if (mix > 1.0f) mix = 1.0f;
        if (mix < -1.0f) mix = -1.0f;
        dst[i] = (int16_t)(mix * 32000.0f);
    }
}

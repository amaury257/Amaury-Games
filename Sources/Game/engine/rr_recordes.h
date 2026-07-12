// rr_recordes.h — Tabela local das 10 melhores pontuações (§7.7), nome de
// 3 letras. Serialização própria (round-trip testado); a persistência em
// arquivo fica com o StateStore (ObjC).
#ifndef RR_RECORDES_H
#define RR_RECORDES_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define RR_RECORDES_N 10

typedef struct { char nome[4]; uint32_t pontos; } rr_recorde;
typedef struct { rr_recorde itens[RR_RECORDES_N]; } rr_recordes;

void   rr_recordes_zerar(rr_recordes *t);
// insere mantendo ordem decrescente; retorna a posição (0-based) ou -1
int    rr_recordes_inserir(rr_recordes *t, const char nome[3], uint32_t pontos);
bool   rr_recordes_entra(const rr_recordes *t, uint32_t pontos);
size_t rr_recordes_tamanho(void);
size_t rr_recordes_salvar(const rr_recordes *t, void *dst, size_t cap);
bool   rr_recordes_carregar(rr_recordes *t, const void *src, size_t len);

#endif

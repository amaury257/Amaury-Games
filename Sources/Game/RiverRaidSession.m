#import "RiverRaidSession.h"
#import <os/lock.h>
#import "rr_jogo.h"
#import "rr_render.h"
#import "rr_audio.h"

@implementation RiverRaidSession {
    os_unfair_lock _trava;      // protege _jogo/_audio/_fb entre emulação e áudio
    rr_jogo _jogo;
    rr_audio _audio;
    uint32_t *_fb;
    uint16_t _seed;
    BOOL _diario;
    RREventosQuadro _eventos;
}

- (instancetype)initComSeed:(uint16_t)seed diario:(BOOL)diario {
    if ((self = [super init])) {
        _trava = OS_UNFAIR_LOCK_INIT;
        _seed = seed;
        _diario = diario;
        _fb = calloc((size_t)RR_LARGURA * RR_ALTURA, sizeof(uint32_t));
    }
    return self;
}

- (void)dealloc {
    free(_fb);
}

- (RRSessaoAVInfo)avInfo {
    return (RRSessaoAVInfo){ RR_LARGURA, RR_ALTURA, RR_FPS, RR_TAXA_AUDIO };
}

- (BOOL)iniciar:(NSError **)erro {
    os_unfair_lock_lock(&_trava);
    rr_jogo_iniciar(&_jogo, _seed, _diario);
    rr_audio_iniciar(&_audio);
    rr_render_quadro(&_jogo, _fb);
    os_unfair_lock_unlock(&_trava);
    return YES;
}

- (void)executarQuadro:(RREstadoEntrada)entrada {
    rr_entrada e = {
        .eixo_x = entrada.analogicoX,
        .eixo_y = entrada.analogicoY,
        .fogo = (entrada.botoes & RRBotaoFogo) != 0,
    };
    if (entrada.botoes & RRBotaoEsquerda) e.eixo_x = -1.0f;
    if (entrada.botoes & RRBotaoDireita)  e.eixo_x = 1.0f;
    if (entrada.botoes & RRBotaoCima)     e.eixo_y = 1.0f;
    if (entrada.botoes & RRBotaoBaixo)    e.eixo_y = -1.0f;

    os_unfair_lock_lock(&_trava);
    rr_jogo_quadro(&_jogo, &e);
    // eventos para haptics/UI, antes de o áudio consumi-los
    if (_jogo.ev_tiro) _eventos.tiro = YES;
    if (_jogo.ev_explosao) _eventos.explosao = YES;
    if (_jogo.ev_vida_extra) _eventos.vidaExtra = YES;
    _eventos.reabastecendo = _jogo.reabastecendo != 0;
    rr_audio_sincronizar(&_audio, &_jogo);
    rr_render_quadro(&_jogo, _fb);
    os_unfair_lock_unlock(&_trava);
}

- (RRQuadroVideo)quadroAtual {
    return (RRQuadroVideo){ _fb, RR_LARGURA, RR_ALTURA, RR_LARGURA * 4 };
}

- (NSInteger)puxarAudio:(int16_t *)destino quadros:(NSInteger)quadros {
    os_unfair_lock_lock(&_trava);
    rr_audio_processar(&_audio, destino, (int)quadros);
    os_unfair_lock_unlock(&_trava);
    return quadros;
}

- (NSData *)salvarEstado:(NSError **)erro {
    size_t cap = rr_estado_tamanho();
    NSMutableData *dados = [NSMutableData dataWithLength:cap];
    os_unfair_lock_lock(&_trava);
    size_t n = rr_estado_salvar(&_jogo, dados.mutableBytes, cap);
    os_unfair_lock_unlock(&_trava);
    if (n != cap) {
        if (erro) *erro = [NSError errorWithDomain:@"RiverRaid" code:1 userInfo:
            @{NSLocalizedDescriptionKey: @"falha ao serializar o estado"}];
        return nil;
    }
    return dados;
}

- (BOOL)carregarEstado:(NSData *)dados erro:(NSError **)erro {
    os_unfair_lock_lock(&_trava);
    bool ok = rr_estado_carregar(&_jogo, dados.bytes, dados.length);
    if (ok) rr_render_quadro(&_jogo, _fb);
    os_unfair_lock_unlock(&_trava);
    if (!ok && erro)
        *erro = [NSError errorWithDomain:@"RiverRaid" code:2 userInfo:
            @{NSLocalizedDescriptionKey: @"save state inválido"}];
    return ok;
}

- (void)reiniciar:(BOOL)completo {
    (void)completo;
    os_unfair_lock_lock(&_trava);
    rr_jogo_iniciar(&_jogo, _seed, _diario);
    rr_audio_iniciar(&_audio);
    rr_render_quadro(&_jogo, _fb);
    os_unfair_lock_unlock(&_trava);
}

- (void)parar {
}

- (uint32_t)pontos {
    os_unfair_lock_lock(&_trava);
    uint32_t p = _jogo.pontos;
    os_unfair_lock_unlock(&_trava);
    return p;
}

- (uint32_t)secao {
    os_unfair_lock_lock(&_trava);
    uint32_t s = _jogo.secao;
    os_unfair_lock_unlock(&_trava);
    return s;
}

- (BOOL)terminou {
    os_unfair_lock_lock(&_trava);
    BOOL fim = _jogo.fase == RR_FASE_FIM;
    os_unfair_lock_unlock(&_trava);
    return fim;
}

- (RREventosQuadro)coletarEventos {
    os_unfair_lock_lock(&_trava);
    RREventosQuadro e = _eventos;
    _eventos = (RREventosQuadro){ NO, NO, _eventos.reabastecendo, NO };
    os_unfair_lock_unlock(&_trava);
    return e;
}

@end

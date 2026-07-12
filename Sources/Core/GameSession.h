// GameSession.h — Protocolo unificado (§6 do prompt mestre).
// O River Raid e, nas Fases 3–4, os cores libretro o implementam;
// a UI e a infraestrutura não sabem a diferença.
#import <Foundation/Foundation.h>

typedef struct {
    int largura, altura;
    double fps, taxaAmostragem;
} RRSessaoAVInfo;

// framebuffer XRGB8888 (bytes B,G,R,A em little-endian ⇒ MTLPixelFormatBGRA8Unorm)
typedef struct {
    const void *pixels;
    int largura, altura, pitchBytes;
} RRQuadroVideo;

typedef NS_OPTIONS(uint32_t, RRBotoes) {
    RRBotaoCima     = 1u << 0,
    RRBotaoBaixo    = 1u << 1,
    RRBotaoEsquerda = 1u << 2,
    RRBotaoDireita  = 1u << 3,
    RRBotaoFogo     = 1u << 4,
    RRBotaoIniciar  = 1u << 5,
};

typedef struct {
    RRBotoes botoes;
    float analogicoX, analogicoY;   // -1..1
} RREstadoEntrada;

// eventos de um quadro, para haptics/UI (coletados e zerados a cada leitura)
typedef struct {
    BOOL tiro, explosao, reabastecendo, vidaExtra;
} RREventosQuadro;

@protocol GameSession <NSObject>
@property (nonatomic, readonly) RRSessaoAVInfo avInfo;
- (BOOL)iniciar:(NSError **)erro;
- (void)executarQuadro:(RREstadoEntrada)entrada;    // avança exatamente 1 frame
- (RRQuadroVideo)quadroAtual;
- (NSInteger)puxarAudio:(int16_t *)destino quadros:(NSInteger)quadros;
- (NSData *)salvarEstado:(NSError **)erro;
- (BOOL)carregarEstado:(NSData *)dados erro:(NSError **)erro;
- (void)reiniciar:(BOOL)completo;
- (void)parar;
@end

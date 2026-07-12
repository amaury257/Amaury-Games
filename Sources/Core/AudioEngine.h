// AudioEngine.h — Áudio via AVAudioSourceNode em modelo pull (D5).
// Na Fase 1 a fonte é o sintetizador do jogo, clocado pelo próprio callback
// de áudio — sem deriva de relógio, sem estalos. O ring buffer + controle
// dinâmico de taxa (para cores libretro, que empurram lotes) entra na Fase 3.
#import <Foundation/Foundation.h>

typedef NSInteger (^RRFonteAudio)(int16_t *destino, NSInteger quadros);

@interface AudioEngine : NSObject
- (instancetype)initComTaxa:(double)taxa fonte:(RRFonteAudio)fonte;
- (BOOL)iniciar:(NSError **)erro;
- (void)pausar;
- (void)retomar;
- (void)parar;
@end

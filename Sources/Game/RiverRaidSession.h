// RiverRaidSession.h — O River Raid visto pela infraestrutura: uma GameSession
// como outra qualquer (§6). Encapsula o motor C determinístico (engine/).
#import <Foundation/Foundation.h>
#import "GameSession.h"

@interface RiverRaidSession : NSObject <GameSession>
- (instancetype)initComSeed:(uint16_t)seed diario:(BOOL)diario;

// leitura segura (thread principal) para HUD externo, fim de jogo e haptics
@property (nonatomic, readonly) uint32_t pontos;
@property (nonatomic, readonly) uint32_t secao;
@property (nonatomic, readonly) BOOL terminou;
- (RREventosQuadro)coletarEventos;   // zera ao ler
@end

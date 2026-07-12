// EmuLoop.h — Thread de emulação dedicada (QoS userInteractive) cadenciada
// por CADisplayLink fixado em 60 Hz (§6). No iPhone 14 (tela 60 Hz) é 1:1;
// em telas ProMotion o range fixo faz cada frame ser apresentado 2×.
#import <Foundation/Foundation.h>

@interface EmuLoop : NSObject
// 'tique' roda na thread de emulação, uma vez por vsync de 60 Hz
- (instancetype)initComTique:(void (^)(void))tique;
- (void)iniciar;
- (void)parar;      // síncrono: retorna após a thread encerrar
@end

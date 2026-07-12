// VideoRenderer.h — Vídeo Metal (D4): textura por frame via replaceRegion,
// amostragem nearest, escala inteira com barras discretas (nunca deformar).
#import <Foundation/Foundation.h>
#import <QuartzCore/CAMetalLayer.h>
#import "GameSession.h"

@interface VideoRenderer : NSObject
- (instancetype)initComCamada:(CAMetalLayer *)camada largura:(int)largura altura:(int)altura;
// cria device/fila/pipeline; o shader é compilado em runtime a partir de fonte
// embutida (não há toolchain Metal offline no Theos/Linux)
- (BOOL)preparar:(NSError **)erro;
- (void)apresentarQuadro:(RRQuadroVideo)quadro;   // chamar na thread de emulação
@end

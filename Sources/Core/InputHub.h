// InputHub.h — Entrada unificada (§7.12): manche virtual FLUTUANTE na metade
// esquerda (aparece onde tocar — preferência do usuário, §15), botão de fogo
// na direita, e gamepad Bluetooth (GCController) mesclado ao toque.
// estadoAtual é seguro para leitura da thread de emulação.
#import <UIKit/UIKit.h>
#import "GameSession.h"

@interface InputHub : UIView
@property (nonatomic, copy) void (^aoPausar)(void);   // botão Menu do gamepad
- (RREstadoEntrada)estadoAtual;
@end

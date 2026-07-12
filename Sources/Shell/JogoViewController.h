#import <UIKit/UIKit.h>

// Sessão de jogo em tela cheia (§7): canvas Metal 160×192 em escala inteira,
// manche flutuante + botão de fogo, quick menu de pausa, fim de jogo com
// entrada de recorde (nome de 3 letras em teclado retrô próprio).
@interface JogoViewController : UIViewController
- (instancetype)initComModoDiario:(BOOL)diario;
@end

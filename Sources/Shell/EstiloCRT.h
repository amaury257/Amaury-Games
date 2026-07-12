// EstiloCRT.h — Direção visual "CRT-noir" (§9): carvão profundo, acentos
// fósforo com parcimônia. Tipografia display própria entra na Fase 5;
// até lá, monoespaçada tratada (peso/tracking) — nunca SF puro sem tratamento.
#import <UIKit/UIKit.h>

@interface EstiloCRT : NSObject
+ (UIColor *)carvao;
+ (UIColor *)fosforo;
+ (UIColor *)ambar;
+ (UIColor *)cinza;
+ (UILabel *)tituloComTexto:(NSString *)texto tamanho:(CGFloat)tamanho;
+ (UIButton *)botaoComTexto:(NSString *)texto;
+ (NSAttributedString *)textoEspacado:(NSString *)texto
                                fonte:(UIFont *)fonte
                                  cor:(UIColor *)cor;
@end

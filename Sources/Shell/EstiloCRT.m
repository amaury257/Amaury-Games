#import "EstiloCRT.h"

@implementation EstiloCRT

+ (UIColor *)carvao  { return [UIColor colorWithRed:0.04 green:0.04 blue:0.05 alpha:1]; }
+ (UIColor *)fosforo { return [UIColor colorWithRed:0.20 green:0.95 blue:0.40 alpha:1]; }
+ (UIColor *)ambar   { return [UIColor colorWithRed:0.98 green:0.72 blue:0.16 alpha:1]; }
+ (UIColor *)cinza   { return [UIColor colorWithWhite:0.62 alpha:1]; }

+ (NSAttributedString *)textoEspacado:(NSString *)texto fonte:(UIFont *)fonte cor:(UIColor *)cor {
    return [[NSAttributedString alloc] initWithString:texto attributes:@{
        NSFontAttributeName: fonte,
        NSForegroundColorAttributeName: cor,
        NSKernAttributeName: @(fonte.pointSize * 0.18),
    }];
}

+ (UILabel *)tituloComTexto:(NSString *)texto tamanho:(CGFloat)tamanho {
    UILabel *l = [[UILabel alloc] init];
    l.attributedText = [self textoEspacado:texto
        fonte:[UIFont monospacedSystemFontOfSize:tamanho weight:UIFontWeightBold]
          cor:[self fosforo]];
    l.textAlignment = NSTextAlignmentCenter;
    // brilho sutil de fósforo (§9)
    l.layer.shadowColor = [self fosforo].CGColor;
    l.layer.shadowRadius = 8;
    l.layer.shadowOpacity = 0.6;
    l.layer.shadowOffset = CGSizeZero;
    return l;
}

+ (UIButton *)botaoComTexto:(NSString *)texto {
    UIButton *b = [UIButton buttonWithType:UIButtonTypeSystem];
    [b setAttributedTitle:[self textoEspacado:texto
        fonte:[UIFont monospacedSystemFontOfSize:17 weight:UIFontWeightSemibold]
          cor:[self fosforo]] forState:UIControlStateNormal];
    b.backgroundColor = [UIColor colorWithWhite:1.0 alpha:0.04];
    b.layer.borderColor = [[self fosforo] colorWithAlphaComponent:0.55].CGColor;
    b.layer.borderWidth = 1.0;
    b.layer.cornerRadius = 6.0;
    b.contentEdgeInsets = UIEdgeInsetsMake(14, 24, 14, 24);   // alvo ≥ 44 pt
    return b;
}

@end

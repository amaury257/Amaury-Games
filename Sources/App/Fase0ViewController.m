#import "Fase0ViewController.h"
#import <dlfcn.h>

// Paleta mínima "CRT-noir" da Fase 0 (a direção visual completa entra na Fase 1/5):
// carvão profundo de fundo, fósforo verde para sucesso, âmbar para falha.
static UIColor *CorCarvao(void)  { return [UIColor colorWithRed:0.04 green:0.04 blue:0.05 alpha:1.0]; }
static UIColor *CorFosforo(void) { return [UIColor colorWithRed:0.20 green:0.95 blue:0.40 alpha:1.0]; }
static UIColor *CorAmbar(void)   { return [UIColor colorWithRed:0.98 green:0.72 blue:0.16 alpha:1.0]; }

@implementation Fase0ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    self.view.backgroundColor = CorCarvao();

    // Resultado do teste crítico da Fase 0: dlopen de dylib embarcada,
    // após a reassinatura do .ipa pelo AltStore.
    BOOL dlopenOk = NO;
    NSString *detalhe = [self executarTesteDlopenComResultado:&dlopenOk];

    UILabel *titulo = [self rotuloComTexto:@"RIVER RAID"
                                     fonte:[UIFont monospacedSystemFontOfSize:34 weight:UIFontWeightBold]
                                       cor:CorFosforo()];
    UILabel *subtitulo = [self rotuloComTexto:@"Fase 0 — gate de toolchain"
                                        fonte:[UIFont monospacedSystemFontOfSize:15 weight:UIFontWeightRegular]
                                          cor:[UIColor colorWithWhite:0.75 alpha:1.0]];

    NSString *versao = [NSBundle.mainBundle objectForInfoDictionaryKey:@"CFBundleShortVersionString"] ?: @"?";
    UILabel *rotuloVersao = [self rotuloComTexto:[NSString stringWithFormat:@"v%@ · Theos/WSL2 · ObjC", versao]
                                           fonte:[UIFont monospacedSystemFontOfSize:12 weight:UIFontWeightRegular]
                                             cor:[UIColor colorWithWhite:0.45 alpha:1.0]];

    UILabel *resultado = [self rotuloComTexto:detalhe
                                        fonte:[UIFont monospacedSystemFontOfSize:14 weight:UIFontWeightMedium]
                                          cor:(dlopenOk ? CorFosforo() : CorAmbar())];

    UILabel *veredito = [self rotuloComTexto:(dlopenOk
                                              ? @"✓ Reassinatura AltStore + dlopen: OK\nD3-A (frameworks embarcados) confirmado"
                                              : @"✗ dlopen falhou — avaliar fallback D3-B\n(ver docs/build.md, seção troubleshooting)")
                                       fonte:[UIFont monospacedSystemFontOfSize:13 weight:UIFontWeightRegular]
                                         cor:(dlopenOk ? CorFosforo() : CorAmbar())];

    UIStackView *pilha = [[UIStackView alloc] initWithArrangedSubviews:@[titulo, subtitulo, rotuloVersao, resultado, veredito]];
    pilha.axis = UILayoutConstraintAxisVertical;
    pilha.alignment = UIStackViewAlignmentCenter;
    pilha.spacing = 14;
    [pilha setCustomSpacing:40 afterView:rotuloVersao];
    pilha.translatesAutoresizingMaskIntoConstraints = NO;
    [self.view addSubview:pilha];

    [NSLayoutConstraint activateConstraints:@[
        [pilha.centerXAnchor constraintEqualToAnchor:self.view.centerXAnchor],
        [pilha.centerYAnchor constraintEqualToAnchor:self.view.centerYAnchor],
        [pilha.leadingAnchor constraintGreaterThanOrEqualToAnchor:self.view.safeAreaLayoutGuide.leadingAnchor constant:24],
        [pilha.trailingAnchor constraintLessThanOrEqualToAnchor:self.view.safeAreaLayoutGuide.trailingAnchor constant:-24],
    ]];
}

// Tenta carregar Frameworks/libdummy.dylib e chamar fase0_mensagem().
// Sucesso prova que o AltStore reassinou o framework embarcado e que o
// mecanismo de carregamento dinâmico dos cores libretro (D3-A) funciona.
- (NSString *)executarTesteDlopenComResultado:(BOOL *)ok {
    *ok = NO;
    NSString *caminho = [NSBundle.mainBundle.privateFrameworksPath
                         stringByAppendingPathComponent:@"libdummy.dylib"];

    if (![NSFileManager.defaultManager fileExistsAtPath:caminho]) {
        return [NSString stringWithFormat:@"dylib ausente do bundle:\n%@", caminho];
    }

    void *handle = dlopen(caminho.fileSystemRepresentation, RTLD_NOW);
    if (!handle) {
        return [NSString stringWithFormat:@"dlopen retornou NULL:\n%s", dlerror()];
    }

    const char *(*mensagem)(void) = dlsym(handle, "fase0_mensagem");
    if (!mensagem) {
        NSString *erro = [NSString stringWithFormat:@"dlsym falhou:\n%s", dlerror()];
        dlclose(handle);
        return erro;
    }

    NSString *texto = [NSString stringWithUTF8String:mensagem()];
    dlclose(handle);
    *ok = YES;
    return texto;
}

- (UILabel *)rotuloComTexto:(NSString *)texto fonte:(UIFont *)fonte cor:(UIColor *)cor {
    UILabel *rotulo = [[UILabel alloc] init];
    rotulo.text = texto;
    rotulo.font = fonte;
    rotulo.textColor = cor;
    rotulo.numberOfLines = 0;
    rotulo.textAlignment = NSTextAlignmentCenter;
    return rotulo;
}

- (UIStatusBarStyle)preferredStatusBarStyle {
    return UIStatusBarStyleLightContent;
}

@end

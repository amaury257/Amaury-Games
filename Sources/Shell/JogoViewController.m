#import "JogoViewController.h"
#import <QuartzCore/CAMetalLayer.h>
#import <CoreHaptics/CoreHaptics.h>
#import "EstiloCRT.h"
#import "GameSession.h"
#import "RiverRaidSession.h"
#import "VideoRenderer.h"
#import "AudioEngine.h"
#import "InputHub.h"
#import "EmuLoop.h"
#import "StateStore.h"
#import "Log.h"
#import "rr_jogo.h"
#import "rr_recordes.h"

// UIView cuja layer é uma CAMetalLayer
@interface RRMetalView : UIView
@end
@implementation RRMetalView
+ (Class)layerClass { return [CAMetalLayer class]; }
@end

@implementation JogoViewController {
    BOOL _diario;
    uint32_t _dataDiario;
    RiverRaidSession *_sessao;
    VideoRenderer *_renderer;
    AudioEngine *_audio;
    InputHub *_input;
    EmuLoop *_loop;
    RRMetalView *_telaMetal;
    NSTimer *_timerUI;
    BOOL _pausado, _fimTratado;
    UIView *_overlayPausa, *_overlayFim;
    CHHapticEngine *_haptics;
    NSMutableString *_nomeRecorde;
    UILabel *_slotsNome;
}

- (instancetype)initComModoDiario:(BOOL)diario {
    if ((self = [super init])) {
        _diario = diario;
        _dataDiario = [StateStore dataLocalAAAAMMDD];
    }
    return self;
}

- (BOOL)prefersStatusBarHidden { return YES; }
- (BOOL)prefersHomeIndicatorAutoHidden { return YES; }
// o jogo é retrato nativo (§7.1); paisagem chega com o SNES na Fase 4
- (UIInterfaceOrientationMask)supportedInterfaceOrientations {
    return UIInterfaceOrientationMaskPortrait;
}

- (void)viewDidLoad {
    [super viewDidLoad];
    self.view.backgroundColor = [EstiloCRT carvao];

    _telaMetal = [[RRMetalView alloc] initWithFrame:CGRectZero];
    [self.view addSubview:_telaMetal];

    uint16_t seed = _diario ? rr_seed_diario(_dataDiario) : RR_SEED_CLASSICO;
    _sessao = [[RiverRaidSession alloc] initComSeed:seed diario:_diario];
    [_sessao iniciar:NULL];

    RRSessaoAVInfo av = _sessao.avInfo;
    _renderer = [[VideoRenderer alloc] initComCamada:(CAMetalLayer *)_telaMetal.layer
                                             largura:av.largura altura:av.altura];
    NSError *erro = nil;
    if (![_renderer preparar:&erro]) {
        [Log registrar:@"erro" modulo:@"Jogo" evento:@"metal_falhou"
                 dados:@{@"erro": erro.localizedDescription ?: @"?"}];
    }

    RiverRaidSession *sessao = _sessao;
    _audio = [[AudioEngine alloc] initComTaxa:av.taxaAmostragem
        fonte:^NSInteger(int16_t *dst, NSInteger quadros) {
            return [sessao puxarAudio:dst quadros:quadros];
        }];
    [_audio iniciar:NULL];

    _input = [[InputHub alloc] initWithFrame:self.view.bounds];
    _input.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    __weak JogoViewController *fraco = self;
    _input.aoPausar = ^{ [fraco alternarPausa]; };
    [self.view addSubview:_input];

    UIButton *pausa = [UIButton buttonWithType:UIButtonTypeSystem];
    [pausa setAttributedTitle:[EstiloCRT textoEspacado:@"II"
        fonte:[UIFont monospacedSystemFontOfSize:18 weight:UIFontWeightBold]
          cor:[EstiloCRT cinza]] forState:UIControlStateNormal];
    [pausa addTarget:self action:@selector(alternarPausa)
    forControlEvents:UIControlEventTouchUpInside];
    pausa.translatesAutoresizingMaskIntoConstraints = NO;
    [self.view addSubview:pausa];
    [NSLayoutConstraint activateConstraints:@[
        [pausa.topAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.topAnchor constant:8],
        [pausa.trailingAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.trailingAnchor constant:-16],
        [pausa.widthAnchor constraintGreaterThanOrEqualToConstant:44],
        [pausa.heightAnchor constraintGreaterThanOrEqualToConstant:44],
    ]];

    [self prepararHaptics];

    // loop de emulação: input → 1 quadro → apresentação (thread dedicada, 60 Hz)
    VideoRenderer *renderer = _renderer;
    InputHub *input = _input;
    _loop = [[EmuLoop alloc] initComTique:^{
        JogoViewController *forte = fraco;
        if (!forte || forte->_pausado) return;
        [sessao executarQuadro:[input estadoAtual]];
        [renderer apresentarQuadro:[sessao quadroAtual]];
    }];

    _timerUI = [NSTimer scheduledTimerWithTimeInterval:0.1 target:self
        selector:@selector(tiqueUI) userInfo:nil repeats:YES];

    [NSNotificationCenter.defaultCenter addObserver:self
        selector:@selector(appSaiu) name:UIApplicationWillResignActiveNotification object:nil];

    [Log registrar:@"info" modulo:@"Jogo" evento:@"sessao_iniciada"
             dados:@{@"modo": _diario ? @"diario" : @"classico", @"seed": @(seed)}];
}

- (void)viewDidLayoutSubviews {
    [super viewDidLayoutSubviews];
    // canvas 160×192 no maior tamanho proporcional dentro da área segura
    CGRect area = UIEdgeInsetsInsetRect(self.view.bounds, self.view.safeAreaInsets);
    CGFloat fator = MIN(area.size.width / RR_LARGURA, area.size.height / RR_ALTURA);
    CGSize tam = CGSizeMake(RR_LARGURA * fator, RR_ALTURA * fator);
    _telaMetal.frame = CGRectMake(area.origin.x + (area.size.width - tam.width) / 2,
                                  area.origin.y + (area.size.height - tam.height) / 2,
                                  tam.width, tam.height);
    CGFloat escala = self.view.window.screen.nativeScale ?: UIScreen.mainScreen.nativeScale;
    ((CAMetalLayer *)_telaMetal.layer).drawableSize =
        CGSizeMake(tam.width * escala, tam.height * escala);
}

- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    // efeito "power-on" de CRT: linha que expande (§9)
    _telaMetal.transform = CGAffineTransformMakeScale(1.0, 0.005);
    _telaMetal.alpha = 0.0;
    [UIView animateWithDuration:0.28 delay:0.05 options:UIViewAnimationOptionCurveEaseOut
        animations:^{
            self->_telaMetal.transform = CGAffineTransformIdentity;
            self->_telaMetal.alpha = 1.0;
        } completion:^(BOOL fim) {
            [self->_loop iniciar];
        }];
}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
    [self encerrar];
}

- (void)dealloc {
    [NSNotificationCenter.defaultCenter removeObserver:self];
}

- (void)encerrar {
    [_timerUI invalidate];
    _timerUI = nil;
    [_loop parar];
    [_audio parar];
    [_sessao parar];
}

// ------------------------------------------------------------------ haptics (§7.12)
- (void)prepararHaptics {
    if (!CHHapticEngine.capabilitiesForHardware.supportsHaptics) return;
    _haptics = [[CHHapticEngine alloc] initAndReturnError:nil];
    [_haptics startAndReturnError:nil];
}

- (void)hapticoComIntensidade:(float)intensidade {
    if (!_haptics) return;
    CHHapticEventParameter *p = [[CHHapticEventParameter alloc]
        initWithParameterID:CHHapticEventParameterIDHapticIntensity value:intensidade];
    CHHapticEvent *ev = [[CHHapticEvent alloc]
        initWithEventType:CHHapticEventTypeHapticTransient
               parameters:@[p] relativeTime:0];
    NSError *erro = nil;
    CHHapticPattern *padrao = [[CHHapticPattern alloc] initWithEvents:@[ev]
                                                           parameters:@[] error:&erro];
    if (!padrao) return;
    id<CHHapticPatternPlayer> tocador = [_haptics createPlayerWithPattern:padrao error:&erro];
    [tocador startAtTime:0 error:nil];
}

// ------------------------------------------------------------------ UI periódica
- (void)tiqueUI {
    RREventosQuadro ev = [_sessao coletarEventos];
    if (ev.tiro) [self hapticoComIntensidade:0.3f];
    if (ev.explosao) [self hapticoComIntensidade:0.9f];
    if (ev.vidaExtra) [self hapticoComIntensidade:0.6f];

    if (_sessao.terminou && !_fimTratado) {
        _fimTratado = YES;
        [self mostrarFimDeJogo];
    }
}

// ------------------------------------------------------------------ pausa
- (void)alternarPausa {
    if (_fimTratado) return;
    if (_pausado) { [self retomar]; return; }
    _pausado = YES;
    [_audio pausar];

    UIView *overlay = [self novoOverlay];
    UILabel *titulo = [EstiloCRT tituloComTexto:@"PAUSA" tamanho:26];
    UIButton *retomar = [EstiloCRT botaoComTexto:@"RETOMAR"];
    [retomar addTarget:self action:@selector(retomar) forControlEvents:UIControlEventTouchUpInside];
    UIButton *reiniciar = [EstiloCRT botaoComTexto:@"REINICIAR"];
    [reiniciar addTarget:self action:@selector(reiniciar) forControlEvents:UIControlEventTouchUpInside];
    UIButton *sair = [EstiloCRT botaoComTexto:@"SAIR"];
    [sair addTarget:self action:@selector(sair) forControlEvents:UIControlEventTouchUpInside];
    [self montarOverlay:overlay comViews:@[titulo, retomar, reiniciar, sair]];
    _overlayPausa = overlay;
}

- (void)retomar {
    [_overlayPausa removeFromSuperview];
    _overlayPausa = nil;
    _pausado = NO;
    [_audio retomar];
}

- (void)reiniciar {
    [_overlayPausa removeFromSuperview];
    _overlayPausa = nil;
    [_overlayFim removeFromSuperview];
    _overlayFim = nil;
    [_sessao reiniciar:YES];
    _fimTratado = NO;
    _pausado = NO;
    [_audio retomar];
}

- (void)sair {
    [self.navigationController popViewControllerAnimated:NO];
}

- (void)appSaiu {
    if (!_pausado && !_fimTratado) [self alternarPausa];
}

// ------------------------------------------------------------------ fim de jogo (§7.7)
- (NSString *)modoRecordes {
    return _diario ? [NSString stringWithFormat:@"diario-%u", _dataDiario] : @"classico";
}

- (void)mostrarFimDeJogo {
    uint32_t pontos = _sessao.pontos;
    [Log registrar:@"info" modulo:@"Jogo" evento:@"fim_de_jogo"
             dados:@{@"pontos": @(pontos), @"secao": @(_sessao.secao)}];

    rr_recordes tabela;
    [[StateStore compartilhado] carregarRecordes:&tabela modo:[self modoRecordes]];

    UIView *overlay = [self novoOverlay];
    _overlayFim = overlay;
    UILabel *titulo = [EstiloCRT tituloComTexto:@"FIM DE JOGO" tamanho:24];
    UILabel *placar = [[UILabel alloc] init];
    placar.attributedText = [EstiloCRT textoEspacado:
        [NSString stringWithFormat:@"%06u PONTOS", pontos]
        fonte:[UIFont monospacedSystemFontOfSize:20 weight:UIFontWeightBold]
          cor:[EstiloCRT ambar]];
    placar.textAlignment = NSTextAlignmentCenter;

    if (rr_recordes_entra(&tabela, pontos)) {
        // entrada de nome com teclado retrô próprio (3 letras)
        _nomeRecorde = [NSMutableString string];
        UILabel *chamada = [[UILabel alloc] init];
        chamada.attributedText = [EstiloCRT textoEspacado:@"NOVO RECORDE · SEU NOME"
            fonte:[UIFont monospacedSystemFontOfSize:13 weight:UIFontWeightRegular]
              cor:[EstiloCRT cinza]];
        chamada.textAlignment = NSTextAlignmentCenter;
        _slotsNome = [[UILabel alloc] init];
        _slotsNome.textAlignment = NSTextAlignmentCenter;
        [self atualizarSlots];

        NSMutableArray<UIView *> *linhas = [NSMutableArray array];
        NSArray<NSString *> *filas = @[@"ABCDEFG", @"HIJKLMN", @"OPQRSTU", @"VWXYZ<>"];
        for (NSString *fila in filas) {
            NSMutableArray<UIView *> *teclas = [NSMutableArray array];
            for (NSUInteger i = 0; i < fila.length; i++) {
                NSString *ch = [fila substringWithRange:NSMakeRange(i, 1)];
                UIButton *tecla = [UIButton buttonWithType:UIButtonTypeSystem];
                NSString *rotulo = [ch isEqualToString:@"<"] ? @"⌫"
                                 : [ch isEqualToString:@">"] ? @"OK" : ch;
                [tecla setAttributedTitle:[EstiloCRT textoEspacado:rotulo
                    fonte:[UIFont monospacedSystemFontOfSize:16 weight:UIFontWeightSemibold]
                      cor:[EstiloCRT fosforo]] forState:UIControlStateNormal];
                tecla.accessibilityLabel = rotulo;
                [tecla addTarget:self action:@selector(teclaRetro:)
                forControlEvents:UIControlEventTouchUpInside];
                [tecla.heightAnchor constraintGreaterThanOrEqualToConstant:44].active = YES;
                [teclas addObject:tecla];
            }
            UIStackView *linha = [[UIStackView alloc] initWithArrangedSubviews:teclas];
            linha.axis = UILayoutConstraintAxisHorizontal;
            linha.distribution = UIStackViewDistributionFillEqually;
            [linhas addObject:linha];
        }
        UIStackView *teclado = [[UIStackView alloc] initWithArrangedSubviews:linhas];
        teclado.axis = UILayoutConstraintAxisVertical;
        teclado.spacing = 2;
        [self montarOverlay:overlay comViews:@[titulo, placar, chamada, _slotsNome, teclado]];
    } else {
        UIButton *denovo = [EstiloCRT botaoComTexto:@"JOGAR DE NOVO"];
        [denovo addTarget:self action:@selector(reiniciar) forControlEvents:UIControlEventTouchUpInside];
        UIButton *sair = [EstiloCRT botaoComTexto:@"SAIR"];
        [sair addTarget:self action:@selector(sair) forControlEvents:UIControlEventTouchUpInside];
        [self montarOverlay:overlay comViews:@[titulo, placar, denovo, sair]];
    }
}

- (void)atualizarSlots {
    NSMutableString *s = [NSMutableString string];
    for (NSUInteger i = 0; i < 3; i++) {
        if (i < _nomeRecorde.length)
            [s appendString:[_nomeRecorde substringWithRange:NSMakeRange(i, 1)]];
        else
            [s appendString:@"_"];
        if (i < 2) [s appendString:@" "];
    }
    _slotsNome.attributedText = [EstiloCRT textoEspacado:s
        fonte:[UIFont monospacedSystemFontOfSize:30 weight:UIFontWeightBold]
          cor:[EstiloCRT fosforo]];
}

- (void)teclaRetro:(UIButton *)tecla {
    NSString *rotulo = tecla.accessibilityLabel;
    if ([rotulo isEqualToString:@"⌫"]) {
        if (_nomeRecorde.length)
            [_nomeRecorde deleteCharactersInRange:NSMakeRange(_nomeRecorde.length - 1, 1)];
    } else if ([rotulo isEqualToString:@"OK"]) {
        if (_nomeRecorde.length < 3) return;
        [self salvarRecorde];
        return;
    } else if (_nomeRecorde.length < 3) {
        [_nomeRecorde appendString:rotulo];
    }
    [self atualizarSlots];
}

- (void)salvarRecorde {
    rr_recordes tabela;
    NSString *modo = [self modoRecordes];
    [[StateStore compartilhado] carregarRecordes:&tabela modo:modo];
    char nome[3] = { 'A', 'A', 'A' };
    for (NSUInteger i = 0; i < 3 && i < _nomeRecorde.length; i++)
        nome[i] = (char)[_nomeRecorde characterAtIndex:i];
    rr_recordes_inserir(&tabela, nome, _sessao.pontos);
    [[StateStore compartilhado] salvarRecordes:&tabela modo:modo];
    [Log registrar:@"info" modulo:@"Jogo" evento:@"recorde_salvo"
             dados:@{@"pontos": @(_sessao.pontos), @"modo": modo}];
    [self sair];
}

// ------------------------------------------------------------------ overlays
- (UIView *)novoOverlay {
    UIView *overlay = [[UIView alloc] initWithFrame:self.view.bounds];
    overlay.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    overlay.backgroundColor = [[EstiloCRT carvao] colorWithAlphaComponent:0.88];
    [self.view addSubview:overlay];
    return overlay;
}

- (void)montarOverlay:(UIView *)overlay comViews:(NSArray<UIView *> *)views {
    UIStackView *pilha = [[UIStackView alloc] initWithArrangedSubviews:views];
    pilha.axis = UILayoutConstraintAxisVertical;
    pilha.alignment = UIStackViewAlignmentFill;
    pilha.spacing = 16;
    pilha.translatesAutoresizingMaskIntoConstraints = NO;
    [overlay addSubview:pilha];
    [NSLayoutConstraint activateConstraints:@[
        [pilha.centerXAnchor constraintEqualToAnchor:overlay.centerXAnchor],
        [pilha.centerYAnchor constraintEqualToAnchor:overlay.centerYAnchor],
        [pilha.widthAnchor constraintEqualToConstant:300],
    ]];
}

@end

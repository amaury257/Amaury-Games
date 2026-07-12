#import "HomeViewController.h"
#import "EstiloCRT.h"
#import "JogoViewController.h"
#import "RecordesViewController.h"
#import "Fase0ViewController.h"
#import "Log.h"

@implementation HomeViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    self.view.backgroundColor = [EstiloCRT carvao];

    UILabel *titulo = [EstiloCRT tituloComTexto:@"RIVER RAID" tamanho:38];
    UILabel *subtitulo = [[UILabel alloc] init];
    subtitulo.attributedText = [EstiloCRT textoEspacado:@"ARCADE PESSOAL · OFFLINE"
        fonte:[UIFont monospacedSystemFontOfSize:12 weight:UIFontWeightRegular]
          cor:[EstiloCRT cinza]];
    subtitulo.textAlignment = NSTextAlignmentCenter;

    UIButton *classico = [EstiloCRT botaoComTexto:@"JOGAR · CLÁSSICO"];
    [classico addTarget:self action:@selector(abrirClassico)
       forControlEvents:UIControlEventTouchUpInside];
    UIButton *diario = [EstiloCRT botaoComTexto:@"DESAFIO DIÁRIO"];
    [diario addTarget:self action:@selector(abrirDiario)
     forControlEvents:UIControlEventTouchUpInside];
    UIButton *recordes = [EstiloCRT botaoComTexto:@"RECORDES"];
    [recordes addTarget:self action:@selector(abrirRecordes)
       forControlEvents:UIControlEventTouchUpInside];
    UIButton *diagnostico = [EstiloCRT botaoComTexto:@"DIAGNÓSTICO"];
    diagnostico.layer.borderColor = [[EstiloCRT cinza] colorWithAlphaComponent:0.4].CGColor;
    [diagnostico setAttributedTitle:[EstiloCRT textoEspacado:@"DIAGNÓSTICO"
        fonte:[UIFont monospacedSystemFontOfSize:13 weight:UIFontWeightRegular]
          cor:[EstiloCRT cinza]] forState:UIControlStateNormal];
    [diagnostico addTarget:self action:@selector(abrirDiagnostico)
          forControlEvents:UIControlEventTouchUpInside];

    NSString *v = [NSBundle.mainBundle objectForInfoDictionaryKey:@"CFBundleShortVersionString"] ?: @"?";
    UILabel *versao = [[UILabel alloc] init];
    versao.attributedText = [EstiloCRT textoEspacado:[NSString stringWithFormat:@"V%@ · FASE 1", v]
        fonte:[UIFont monospacedSystemFontOfSize:10 weight:UIFontWeightRegular]
          cor:[[EstiloCRT cinza] colorWithAlphaComponent:0.5]];
    versao.textAlignment = NSTextAlignmentCenter;

    UIStackView *pilha = [[UIStackView alloc] initWithArrangedSubviews:
        @[titulo, subtitulo, classico, diario, recordes, diagnostico, versao]];
    pilha.axis = UILayoutConstraintAxisVertical;
    pilha.alignment = UIStackViewAlignmentFill;
    pilha.spacing = 16;
    [pilha setCustomSpacing:6 afterView:titulo];
    [pilha setCustomSpacing:44 afterView:subtitulo];
    [pilha setCustomSpacing:28 afterView:recordes];
    pilha.translatesAutoresizingMaskIntoConstraints = NO;
    [self.view addSubview:pilha];
    [NSLayoutConstraint activateConstraints:@[
        [pilha.centerXAnchor constraintEqualToAnchor:self.view.centerXAnchor],
        [pilha.centerYAnchor constraintEqualToAnchor:self.view.centerYAnchor],
        [pilha.widthAnchor constraintEqualToConstant:280],
    ]];
}

- (void)abrirClassico {
    [Log registrar:@"info" modulo:@"Home" evento:@"iniciar_classico" dados:@{}];
    [self.navigationController pushViewController:
        [[JogoViewController alloc] initComModoDiario:NO] animated:NO];
}

- (void)abrirDiario {
    [Log registrar:@"info" modulo:@"Home" evento:@"iniciar_diario" dados:@{}];
    [self.navigationController pushViewController:
        [[JogoViewController alloc] initComModoDiario:YES] animated:NO];
}

- (void)abrirRecordes {
    [self.navigationController pushViewController:
        [[RecordesViewController alloc] init] animated:YES];
}

- (void)abrirDiagnostico {
    [self.navigationController pushViewController:
        [[Fase0ViewController alloc] init] animated:YES];
}

@end

#import "RecordesViewController.h"
#import "EstiloCRT.h"
#import "StateStore.h"
#import "rr_recordes.h"

@implementation RecordesViewController {
    UISegmentedControl *_seletor;
    UIStackView *_linhas;
}

- (void)viewDidLoad {
    [super viewDidLoad];
    self.view.backgroundColor = [EstiloCRT carvao];

    UILabel *titulo = [EstiloCRT tituloComTexto:@"RECORDES" tamanho:28];

    _seletor = [[UISegmentedControl alloc] initWithItems:@[@"CLÁSSICO", @"DIÁRIO · HOJE"]];
    _seletor.selectedSegmentIndex = 0;
    _seletor.selectedSegmentTintColor = [[EstiloCRT fosforo] colorWithAlphaComponent:0.25];
    [_seletor setTitleTextAttributes:@{
        NSForegroundColorAttributeName: [EstiloCRT cinza],
        NSFontAttributeName: [UIFont monospacedSystemFontOfSize:12 weight:UIFontWeightSemibold],
    } forState:UIControlStateNormal];
    [_seletor setTitleTextAttributes:@{
        NSForegroundColorAttributeName: [EstiloCRT fosforo],
    } forState:UIControlStateSelected];
    [_seletor addTarget:self action:@selector(recarregar) forControlEvents:UIControlEventValueChanged];

    _linhas = [[UIStackView alloc] init];
    _linhas.axis = UILayoutConstraintAxisVertical;
    _linhas.spacing = 8;

    UIButton *voltar = [EstiloCRT botaoComTexto:@"VOLTAR"];
    [voltar addTarget:self action:@selector(voltar) forControlEvents:UIControlEventTouchUpInside];

    UIStackView *pilha = [[UIStackView alloc] initWithArrangedSubviews:
        @[titulo, _seletor, _linhas, voltar]];
    pilha.axis = UILayoutConstraintAxisVertical;
    pilha.spacing = 24;
    pilha.translatesAutoresizingMaskIntoConstraints = NO;
    [self.view addSubview:pilha];
    [NSLayoutConstraint activateConstraints:@[
        [pilha.centerXAnchor constraintEqualToAnchor:self.view.centerXAnchor],
        [pilha.centerYAnchor constraintEqualToAnchor:self.view.centerYAnchor],
        [pilha.widthAnchor constraintEqualToConstant:300],
    ]];

    [self recarregar];
}

- (void)recarregar {
    for (UIView *v in _linhas.arrangedSubviews) [v removeFromSuperview];

    NSString *modo = _seletor.selectedSegmentIndex == 0
        ? @"classico"
        : [NSString stringWithFormat:@"diario-%u", [StateStore dataLocalAAAAMMDD]];
    rr_recordes tabela;
    [[StateStore compartilhado] carregarRecordes:&tabela modo:modo];

    BOOL vazio = YES;
    for (int i = 0; i < RR_RECORDES_N; i++) {
        if (tabela.itens[i].pontos == 0) continue;
        vazio = NO;
        UILabel *linha = [[UILabel alloc] init];
        NSString *texto = [NSString stringWithFormat:@"%2d  %-3s  %06u",
                           i + 1, tabela.itens[i].nome, tabela.itens[i].pontos];
        linha.attributedText = [EstiloCRT textoEspacado:texto
            fonte:[UIFont monospacedSystemFontOfSize:17 weight:UIFontWeightMedium]
              cor:(i == 0 ? [EstiloCRT ambar] : [EstiloCRT fosforo])];
        linha.textAlignment = NSTextAlignmentCenter;
        [_linhas addArrangedSubview:linha];
    }
    if (vazio) {
        UILabel *nada = [[UILabel alloc] init];
        nada.attributedText = [EstiloCRT textoEspacado:@"SEM RECORDES AINDA"
            fonte:[UIFont monospacedSystemFontOfSize:14 weight:UIFontWeightRegular]
              cor:[EstiloCRT cinza]];
        nada.textAlignment = NSTextAlignmentCenter;
        [_linhas addArrangedSubview:nada];
    }
}

- (void)voltar {
    [self.navigationController popViewControllerAnimated:YES];
}

@end

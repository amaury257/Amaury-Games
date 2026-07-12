#import "InputHub.h"
#import <GameController/GameController.h>
#import <os/lock.h>

static const CGFloat kRaioManche = 44.0;   // alcance do manche em pontos

@implementation InputHub {
    os_unfair_lock _trava;
    float _eixoX, _eixoY;
    BOOL _fogo;

    UITouch *_toqueManche;
    CGPoint _origemManche;
    UITouch *_toqueFogo;

    CAShapeLayer *_base, *_topo;    // manche flutuante
    CAShapeLayer *_botaoFogo;
}

- (instancetype)initWithFrame:(CGRect)frame {
    if ((self = [super initWithFrame:frame])) {
        _trava = OS_UNFAIR_LOCK_INIT;
        self.multipleTouchEnabled = YES;
        self.backgroundColor = UIColor.clearColor;

        _base = [self circuloComRaio:kRaioManche espessura:2.0];
        _topo = [self circuloPreenchidoComRaio:18.0];
        _base.hidden = _topo.hidden = YES;
        [self.layer addSublayer:_base];
        [self.layer addSublayer:_topo];

        _botaoFogo = [self circuloComRaio:44.0 espessura:2.0];
        [self.layer addSublayer:_botaoFogo];

        [NSNotificationCenter.defaultCenter addObserver:self
            selector:@selector(gamepadConectou:)
            name:GCControllerDidConnectNotification object:nil];
        for (GCController *c in GCController.controllers) [self configurarGamepad:c];
    }
    return self;
}

- (void)dealloc {
    [NSNotificationCenter.defaultCenter removeObserver:self];
}

- (CAShapeLayer *)circuloComRaio:(CGFloat)r espessura:(CGFloat)e {
    CAShapeLayer *l = [CAShapeLayer layer];
    l.path = [UIBezierPath bezierPathWithOvalInRect:CGRectMake(-r, -r, 2 * r, 2 * r)].CGPath;
    l.fillColor = [UIColor colorWithWhite:1.0 alpha:0.06].CGColor;
    l.strokeColor = [UIColor colorWithRed:0.20 green:0.95 blue:0.40 alpha:0.5].CGColor;
    l.lineWidth = e;
    return l;
}

- (CAShapeLayer *)circuloPreenchidoComRaio:(CGFloat)r {
    CAShapeLayer *l = [CAShapeLayer layer];
    l.path = [UIBezierPath bezierPathWithOvalInRect:CGRectMake(-r, -r, 2 * r, 2 * r)].CGPath;
    l.fillColor = [UIColor colorWithRed:0.20 green:0.95 blue:0.40 alpha:0.35].CGColor;
    return l;
}

- (void)layoutSubviews {
    [super layoutSubviews];
    // botão de fogo fixo no canto inferior direito (área ≥ 44 pt)
    _botaoFogo.position = CGPointMake(self.bounds.size.width - 76,
                                      self.bounds.size.height - 96);
}

// ------------------------------------------------------------------ toques
- (void)touchesBegan:(NSSet<UITouch *> *)toques withEvent:(UIEvent *)evento {
    for (UITouch *t in toques) {
        CGPoint p = [t locationInView:self];
        if (p.x < self.bounds.size.width * 0.5 && !_toqueManche) {
            _toqueManche = t;
            _origemManche = p;
            [CATransaction begin];
            [CATransaction setDisableActions:YES];
            _base.position = p; _topo.position = p;
            _base.hidden = _topo.hidden = NO;
            [CATransaction commit];
        } else if (p.x >= self.bounds.size.width * 0.5 && !_toqueFogo) {
            _toqueFogo = t;
            os_unfair_lock_lock(&_trava);
            _fogo = YES;
            os_unfair_lock_unlock(&_trava);
            _botaoFogo.fillColor = [UIColor colorWithRed:0.98 green:0.72 blue:0.16 alpha:0.35].CGColor;
        }
    }
}

- (void)touchesMoved:(NSSet<UITouch *> *)toques withEvent:(UIEvent *)evento {
    if (!_toqueManche || ![toques containsObject:_toqueManche]) return;
    CGPoint p = [_toqueManche locationInView:self];
    CGFloat dx = (p.x - _origemManche.x) / kRaioManche;
    CGFloat dy = (p.y - _origemManche.y) / kRaioManche;
    dx = MAX(-1.0, MIN(1.0, dx));
    dy = MAX(-1.0, MIN(1.0, dy));
    os_unfair_lock_lock(&_trava);
    _eixoX = (float)dx;
    _eixoY = (float)-dy;   // tela: y cresce para baixo; manche: cima = acelera
    os_unfair_lock_unlock(&_trava);
    [CATransaction begin];
    [CATransaction setDisableActions:YES];
    _topo.position = CGPointMake(_origemManche.x + dx * kRaioManche,
                                 _origemManche.y + dy * kRaioManche);
    [CATransaction commit];
}

- (void)encerrarToques:(NSSet<UITouch *> *)toques {
    for (UITouch *t in toques) {
        if (t == _toqueManche) {
            _toqueManche = nil;
            _base.hidden = _topo.hidden = YES;
            os_unfair_lock_lock(&_trava);
            _eixoX = _eixoY = 0;
            os_unfair_lock_unlock(&_trava);
        }
        if (t == _toqueFogo) {
            _toqueFogo = nil;
            os_unfair_lock_lock(&_trava);
            _fogo = NO;
            os_unfair_lock_unlock(&_trava);
            _botaoFogo.fillColor = [UIColor colorWithWhite:1.0 alpha:0.06].CGColor;
        }
    }
}

- (void)touchesEnded:(NSSet<UITouch *> *)t withEvent:(UIEvent *)e { [self encerrarToques:t]; }
- (void)touchesCancelled:(NSSet<UITouch *> *)t withEvent:(UIEvent *)e { [self encerrarToques:t]; }

// ------------------------------------------------------------------ gamepad
- (void)gamepadConectou:(NSNotification *)n {
    [self configurarGamepad:n.object];
}

- (void)configurarGamepad:(GCController *)c {
    __weak InputHub *fraco = self;
    c.extendedGamepad.buttonMenu.pressedChangedHandler =
        ^(GCControllerButtonInput *b, float v, BOOL pressionado) {
            if (pressionado) dispatch_async(dispatch_get_main_queue(), ^{
                InputHub *forte = fraco;
                if (forte && forte.aoPausar) forte.aoPausar();
            });
        };
}

// ------------------------------------------------------------------ leitura (thread de emulação)
- (RREstadoEntrada)estadoAtual {
    RREstadoEntrada e = {0};
    os_unfair_lock_lock(&_trava);
    e.analogicoX = _eixoX;
    e.analogicoY = _eixoY;
    if (_fogo) e.botoes |= RRBotaoFogo;
    os_unfair_lock_unlock(&_trava);

    // gamepad mesclado (polling imediatamente antes do quadro, §8.4)
    GCController *c = GCController.controllers.firstObject;
    GCExtendedGamepad *g = c.extendedGamepad;
    if (g) {
        float gx = g.leftThumbstick.xAxis.value;
        float gy = g.leftThumbstick.yAxis.value;
        if (g.dpad.left.pressed)  gx = -1.0f;
        if (g.dpad.right.pressed) gx = 1.0f;
        if (g.dpad.up.pressed)    gy = 1.0f;
        if (g.dpad.down.pressed)  gy = -1.0f;
        if (fabsf(gx) > fabsf(e.analogicoX)) e.analogicoX = gx;
        if (fabsf(gy) > fabsf(e.analogicoY)) e.analogicoY = gy;
        if (g.buttonA.pressed) e.botoes |= RRBotaoFogo;
    }
    return e;
}

@end

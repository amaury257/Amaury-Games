#import "EmuLoop.h"
#import <QuartzCore/CADisplayLink.h>

@implementation EmuLoop {
    void (^_tique)(void);
    NSThread *_thread;
    CADisplayLink *_link;
    volatile BOOL _rodando;
    dispatch_semaphore_t _encerrou;
}

- (instancetype)initComTique:(void (^)(void))tique {
    if ((self = [super init])) {
        _tique = [tique copy];
        _encerrou = dispatch_semaphore_create(0);
    }
    return self;
}

- (void)iniciar {
    if (_rodando) return;
    _rodando = YES;
    _thread = [[NSThread alloc] initWithTarget:self selector:@selector(corpo) object:nil];
    _thread.name = @"emulacao";
    _thread.qualityOfService = NSQualityOfServiceUserInteractive;
    [_thread start];
}

- (void)corpo {
    _link = [CADisplayLink displayLinkWithTarget:self selector:@selector(vsync)];
    _link.preferredFrameRateRange = CAFrameRateRangeMake(60, 60, 60);
    [_link addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSRunLoopCommonModes];
    while (_rodando &&
           [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode
                                    beforeDate:[NSDate dateWithTimeIntervalSinceNow:0.1]]) {
    }
    [_link invalidate];
    _link = nil;
    dispatch_semaphore_signal(_encerrou);
}

- (void)vsync {
    if (_rodando && _tique) _tique();
}

- (void)parar {
    if (!_rodando) return;
    _rodando = NO;
    dispatch_semaphore_wait(_encerrou, dispatch_time(DISPATCH_TIME_NOW, (int64_t)(2 * NSEC_PER_SEC)));
    _thread = nil;
}

@end

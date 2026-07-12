#import "AudioEngine.h"
#import <AVFoundation/AVFoundation.h>

@implementation AudioEngine {
    AVAudioEngine *_engine;
    AVAudioSourceNode *_fonteNode;
    RRFonteAudio _fonte;
    double _taxa;
    int16_t *_tmp;
    NSInteger _tmpCap;
}

- (instancetype)initComTaxa:(double)taxa fonte:(RRFonteAudio)fonte {
    if ((self = [super init])) {
        _taxa = taxa;
        _fonte = [fonte copy];
        _tmpCap = 8192;
        _tmp = malloc(sizeof(int16_t) * (size_t)_tmpCap);
    }
    return self;
}

- (void)dealloc {
    free(_tmp);
}

- (BOOL)iniciar:(NSError **)erro {
    // sessão .ambient: respeita o botão de silêncio (§7.9)
    AVAudioSession *sessao = [AVAudioSession sharedInstance];
    [sessao setCategory:AVAudioSessionCategoryAmbient error:nil];
    [sessao setActive:YES error:nil];

    _engine = [[AVAudioEngine alloc] init];
    AVAudioFormat *formato = [[AVAudioFormat alloc]
        initStandardFormatWithSampleRate:_taxa channels:1];

    RRFonteAudio fonte = _fonte;
    int16_t *tmp = _tmp;
    NSInteger tmpCap = _tmpCap;
    _fonteNode = [[AVAudioSourceNode alloc] initWithFormat:formato
        renderBlock:^OSStatus(BOOL *silencio, const AudioTimeStamp *ts,
                              AVAudioFrameCount quadros, AudioBufferList *saida) {
            (void)ts;
            float *dst = (float *)saida->mBuffers[0].mData;
            NSInteger pedidos = (NSInteger)quadros;
            if (pedidos > tmpCap) pedidos = tmpCap;
            NSInteger obtidos = fonte ? fonte(tmp, pedidos) : 0;
            for (NSInteger i = 0; i < (NSInteger)quadros; i++)
                dst[i] = (i < obtidos) ? (float)tmp[i] / 32768.0f : 0.0f;
            *silencio = (obtidos == 0);
            return noErr;
        }];

    [_engine attachNode:_fonteNode];
    [_engine connect:_fonteNode to:_engine.mainMixerNode format:formato];
    _engine.mainMixerNode.outputVolume = 1.0;
    return [_engine startAndReturnError:erro];
}

- (void)pausar  { [_engine pause]; }
- (void)retomar { [_engine startAndReturnError:nil]; }

- (void)parar {
    [_engine stop];
    _engine = nil;
    _fonteNode = nil;
}

@end

#import "StateStore.h"
#import "Log.h"

@implementation StateStore

+ (instancetype)compartilhado {
    static StateStore *unico;
    static dispatch_once_t once;
    dispatch_once(&once, ^{ unico = [[StateStore alloc] init]; });
    return unico;
}

- (NSURL *)pastaRecordes {
    NSURL *docs = [NSFileManager.defaultManager URLsForDirectory:NSDocumentDirectory
                                                       inDomains:NSUserDomainMask].firstObject;
    NSURL *pasta = [docs URLByAppendingPathComponent:@"recordes" isDirectory:YES];
    [NSFileManager.defaultManager createDirectoryAtURL:pasta
                           withIntermediateDirectories:YES attributes:nil error:nil];
    return pasta;
}

- (NSURL *)urlParaModo:(NSString *)modo {
    return [[self pastaRecordes] URLByAppendingPathComponent:
            [NSString stringWithFormat:@"%@.bin", modo]];
}

- (void)carregarRecordes:(rr_recordes *)tabela modo:(NSString *)modo {
    rr_recordes_zerar(tabela);
    NSData *dados = [NSData dataWithContentsOfURL:[self urlParaModo:modo]];
    if (dados && !rr_recordes_carregar(tabela, dados.bytes, dados.length)) {
        rr_recordes_zerar(tabela);
        [Log registrar:@"aviso" modulo:@"StateStore" evento:@"recordes_corrompidos"
                 dados:@{@"modo": modo}];
    }
}

- (void)salvarRecordes:(const rr_recordes *)tabela modo:(NSString *)modo {
    size_t cap = rr_recordes_tamanho();
    NSMutableData *dados = [NSMutableData dataWithLength:cap];
    if (rr_recordes_salvar(tabela, dados.mutableBytes, cap) == cap) {
        [dados writeToURL:[self urlParaModo:modo] atomically:YES];
    }
}

+ (uint32_t)dataLocalAAAAMMDD {
    NSDateComponents *c = [NSCalendar.currentCalendar
        components:NSCalendarUnitYear | NSCalendarUnitMonth | NSCalendarUnitDay
          fromDate:[NSDate date]];
    return (uint32_t)(c.year * 10000 + c.month * 100 + c.day);
}

@end

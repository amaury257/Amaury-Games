// StateStore.h — Persistência local no sandbox (§6): recordes por modo.
// Save states de sessão e SRAM entram nas Fases 3–4.
#import <Foundation/Foundation.h>
#import "rr_recordes.h"

@interface StateStore : NSObject
+ (instancetype)compartilhado;
// modo: "classico" ou "diario-AAAAMMDD" (recordes do Diário separados por dia, §7.11)
- (void)carregarRecordes:(rr_recordes *)tabela modo:(NSString *)modo;
- (void)salvarRecordes:(const rr_recordes *)tabela modo:(NSString *)modo;
+ (uint32_t)dataLocalAAAAMMDD;
@end

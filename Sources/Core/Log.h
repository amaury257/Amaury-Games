// Log.h — Log JSON estruturado em arquivo rotacionado no sandbox (§12).
// Formato por linha: {"ts":…, "nivel":…, "modulo":…, "evento":…, "dados":…}
// NUNCA envia nada pela rede (offline absoluto, §4).
#import <Foundation/Foundation.h>

@interface Log : NSObject
+ (void)registrar:(NSString *)nivel
           modulo:(NSString *)modulo
           evento:(NSString *)evento
            dados:(NSDictionary *)dados;
@end

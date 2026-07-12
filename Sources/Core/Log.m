#import "Log.h"

static const unsigned long long kLimiteBytes = 256 * 1024;

@implementation Log

+ (dispatch_queue_t)fila {
    static dispatch_queue_t fila;
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        fila = dispatch_queue_create("log.riverraid", DISPATCH_QUEUE_SERIAL);
    });
    return fila;
}

+ (NSString *)caminho {
    NSURL *docs = [NSFileManager.defaultManager URLsForDirectory:NSDocumentDirectory
                                                       inDomains:NSUserDomainMask].firstObject;
    NSURL *pasta = [docs URLByAppendingPathComponent:@"logs" isDirectory:YES];
    [NSFileManager.defaultManager createDirectoryAtURL:pasta
                           withIntermediateDirectories:YES attributes:nil error:nil];
    return [pasta URLByAppendingPathComponent:@"app.log"].path;
}

+ (void)registrar:(NSString *)nivel modulo:(NSString *)modulo
           evento:(NSString *)evento dados:(NSDictionary *)dados {
    NSDictionary *linha = @{
        @"ts": @([NSDate date].timeIntervalSince1970),
        @"nivel": nivel ?: @"info",
        @"modulo": modulo ?: @"",
        @"evento": evento ?: @"",
        @"dados": dados ?: @{},
    };
    NSData *json = [NSJSONSerialization dataWithJSONObject:linha options:0 error:nil];
    if (!json) return;
    dispatch_async([self fila], ^{
        NSString *caminho = [self caminho];
        NSFileManager *fm = NSFileManager.defaultManager;
        NSDictionary *attr = [fm attributesOfItemAtPath:caminho error:nil];
        if (attr && attr.fileSize > kLimiteBytes) {
            NSString *antigo = [caminho stringByAppendingString:@".1"];
            [fm removeItemAtPath:antigo error:nil];
            [fm moveItemAtPath:caminho toPath:antigo error:nil];
        }
        if (![fm fileExistsAtPath:caminho])
            [fm createFileAtPath:caminho contents:nil attributes:nil];
        NSFileHandle *h = [NSFileHandle fileHandleForWritingAtPath:caminho];
        [h seekToEndOfFile];
        [h writeData:json];
        [h writeData:[NSData dataWithBytes:"\n" length:1]];
        [h closeFile];
    });
}

@end

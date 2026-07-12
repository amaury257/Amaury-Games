#import "AppDelegate.h"
#import "Fase0ViewController.h"

@implementation AppDelegate

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    self.window = [[UIWindow alloc] initWithFrame:UIScreen.mainScreen.bounds];
    self.window.rootViewController = [[Fase0ViewController alloc] init];
    [self.window makeKeyAndVisible];
    return YES;
}

@end

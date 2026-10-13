#import <AppKit/AppKit.h>
#import <GameController/GameController.h>
#include <iostream>
int main(int argc,char**){@autoreleasepool {
    const bool app=argc==2;
    if(app){[NSApplication sharedApplication];[NSApp setActivationPolicy:NSApplicationActivationPolicyProhibited];}
    const BOOL previous=GCController.shouldMonitorBackgroundEvents;
    GCController.shouldMonitorBackgroundEvents=YES;
    NSMutableArray* notices=[NSMutableArray array];
    auto observer=[[NSNotificationCenter defaultCenter] addObserverForName:GCControllerDidConnectNotification object:nil queue:nil usingBlock:^(NSNotification* n){
        GCController* c=n.object;[notices addObject:@{@"vendor":c.vendorName?:[NSNull null],@"category":c.productCategory?:[NSNull null]}];}];
    [[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:5]];
    NSMutableArray* current=[NSMutableArray array];
    for(GCController* c in GCController.controllers)[current addObject:@{@"vendor":c.vendorName?:[NSNull null],@"category":c.productCategory?:[NSNull null],@"extended_gamepad":@(c.extendedGamepad!=nil),@"is_snapshot":@(c.isSnapshot)}];
    NSData* data=[NSJSONSerialization dataWithJSONObject:@{@"scope":@"GC-only,noIOHIDopen/supportpredicatecalls",@"application_initialized":@(app),@"background_before":@(previous),@"background_selected":@YES,@"notices":notices,@"controllers":current} options:0 error:nil];
    std::cout.write(static_cast<const char*>(data.bytes),data.length);std::cout<<'\n';
    [[NSNotificationCenter defaultCenter] removeObserver:observer];GCController.shouldMonitorBackgroundEvents=previous;return 0;
}}

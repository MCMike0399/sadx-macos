#include "platform/mac/mac_input.hpp"
#import <IOKit/hidsystem/IOHIDLib.h>
#include <chrono>
#include <fstream>
#include <memory>
#include <unordered_map>
#include <vector>
namespace {
using Clock=std::chrono::steady_clock;
struct Slot {
    __strong GCController* controller;
    __strong dispatch_queue_t previous_queue;
    void (^ __strong previous_handler)(GCExtendedGamepad*,GCControllerElement*)=nil;
    sadx::input::InputState input;
    sadx::input::mac::InputAdapter adapter{input};
    unsigned index=0;
    std::uint64_t callbacks=0;
    std::string previous;
};
}
@interface SADXControllerTester : NSObject <NSApplicationDelegate,NSWindowDelegate> {
    NSWindow* _window;
    NSTextView* _details;
    NSTimer* _timer;
    NSMutableArray* _observers;
    std::unordered_map<const void*,std::unique_ptr<Slot>> _slots;
    std::unique_ptr<sadx::SseContext> _arithmetic;
    sadx::InputModAxesConstants _constants;
    std::ofstream _log;
    Clock::time_point _start;
    BOOL _background;
    bool _cleaned;
    std::uint64_t _samples;
}
- (BOOL)configureLog:(const char*)log constants:(const char*)constants mxcsr:(std::uint32_t)mxcsr;
- (void)tick;
- (void)record:(NSMutableDictionary*)record;
- (void)cleanup;
@end
@implementation SADXControllerTester
- (BOOL)configureLog:(const char*)path constants:(const char*)constants mxcsr:(std::uint32_t)mxcsr {
    std::ifstream file(constants,std::ios::binary);std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(file),{}};
    const auto parsed=sadx::decode_input_mod_axes_constants(bytes.data(),bytes.size());if(!parsed)return NO;
    _constants=*parsed;_arithmetic=std::make_unique<sadx::SseContext>(mxcsr);_log.open(path,std::ios::out|std::ios::trunc);
    _start=Clock::now();_cleaned=false;_samples=0;return _log.good();
}
- (void)record:(NSMutableDictionary*)record {
    record[@"elapsed_ns"]=@(std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now()-_start).count());
    record[@"app_active"]=@(NSApp.active);
    NSData* data=[NSJSONSerialization dataWithJSONObject:record options:NSJSONWritingSortedKeys error:nil];
    if(!data)return;_log.write(static_cast<const char*>(data.bytes),data.length);_log<<'\n'<<std::flush;
}
- (void)applicationDidFinishLaunching:(NSNotification*)notification {
    (void)notification;
    _background=GCController.shouldMonitorBackgroundEvents;GCController.shouldMonitorBackgroundEvents=YES;
    [self record:[@{@"event":@"begin",@"mode":@"realNSApplicationforeground",@"mxcsr_selected":@(_arithmetic->mxcsr()),@"hid_listen_access":@(IOHIDCheckAccess(kIOHIDRequestTypeListenEvent)),@"background_before":@(_background),@"diagnostic_hz":@30,@"scope":@"controllercapture,notgameplayor63FPStiming"} mutableCopy]];
    _window=[[NSWindow alloc] initWithContentRect:NSMakeRect(0,0,880,620) styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable|NSWindowStyleMaskMiniaturizable|NSWindowStyleMaskResizable backing:NSBackingStoreBuffered defer:NO];
    _window.title=@"Native SADX — Controller input test";_window.delegate=self;
    NSScrollView* scroll=[[NSScrollView alloc] initWithFrame:_window.contentView.bounds];scroll.autoresizingMask=NSViewWidthSizable|NSViewHeightSizable;scroll.hasVerticalScroller=YES;
    _details=[[NSTextView alloc] initWithFrame:scroll.bounds];_details.editable=NO;_details.font=[NSFont monospacedSystemFontOfSize:15 weight:NSFontWeightRegular];_details.autoresizingMask=NSViewWidthSizable|NSViewHeightSizable;
    scroll.documentView=_details;[_window.contentView addSubview:scroll];[_window center];[_window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
    _observers=[NSMutableArray array];__weak SADXControllerTester* weak=self;
    for(NSNotificationName name in @[GCControllerDidConnectNotification,GCControllerDidDisconnectNotification,NSApplicationDidBecomeActiveNotification,NSApplicationDidResignActiveNotification]) {
        id observer=[[NSNotificationCenter defaultCenter] addObserverForName:name object:nil queue:[NSOperationQueue mainQueue] usingBlock:^(NSNotification* note){
            [weak record:[@{@"event":@"notification",@"name":note.name} mutableCopy]];[weak tick];}];[_observers addObject:observer];
    }
    _timer=[NSTimer timerWithTimeInterval:1.0/30 target:self selector:@selector(tick) userInfo:nil repeats:YES];
    [[NSRunLoop mainRunLoop] addTimer:_timer forMode:NSRunLoopCommonModes];[self tick];
}
- (void)tick {
    if(_cleaned)return;
    if((++_samples%150)==0)[self record:[@{@"event":@"heartbeat",@"samples":@(_samples),@"controller_count":@(GCController.controllers.count)} mutableCopy]];
    NSMutableString* text=[NSMutableString stringWithFormat:@"Controller input test — %@\n\nKeep this window in front. Press one button or move a stick.\nThe values below should change immediately.\n\n",NSApp.active?@"ACTIVE":@"INACTIVE"];
    unsigned visible=0;
    for(GCController* controller in GCController.controllers) {
        const auto key=(__bridge const void*)controller;
        if(!_slots.count(key)) {
            auto slot=std::make_unique<Slot>();slot->controller=controller;slot->previous_queue=controller.handlerQueue;
            slot->previous_handler=controller.extendedGamepad.valueChangedHandler;
            slot->index=static_cast<unsigned>(_slots.size());controller.handlerQueue=dispatch_get_main_queue();
            __weak SADXControllerTester* weak=self;
            if(controller.extendedGamepad)controller.extendedGamepad.valueChangedHandler=^(GCExtendedGamepad*,GCControllerElement* element){
                auto strong=weak;if(!strong)return;
                const auto found=strong->_slots.find(key);if(found==strong->_slots.end())return;++found->second->callbacks;
                [strong record:[@{@"event":@"gc_value_callback",@"controller_index":@(found->second->index),@"element":element.localizedName?:[NSNull null]} mutableCopy]];
                [strong tick];
            };
            _slots[key]=std::move(slot);
        }
        auto& slot=*_slots[key];++visible;
        auto profile=controller.extendedGamepad;
        [text appendFormat:@"Controller %u: %@ / %@\nCallbacks: %llu   Extended profile: %@\n",slot.index,controller.vendorName?:@"unknown",controller.productCategory?:@"unknown",(unsigned long long)slot.callbacks,profile?@"YES":@"NO"];
        if(!profile)continue;
        const bool accepted=slot.adapter.controller_sample(controller);
        auto record=[NSMutableDictionary dictionary];record[@"event"]=@"gc_snapshot";record[@"controller_index"]=@(slot.index);record[@"vendor"]=controller.vendorName?:[NSNull null];record[@"category"]=controller.productCategory?:[NSNull null];
        record[@"is_snapshot"]=@(controller.isSnapshot);record[@"has_remapped_elements"]=@(controller.physicalInputProfile.hasRemappedElements);record[@"callbacks"]=@(slot.callbacks);
        record[@"left_apple"]=@[@(profile.leftThumbstick.xAxis.value),@(profile.leftThumbstick.yAxis.value)];record[@"right_apple"]=@[@(profile.rightThumbstick.xAxis.value),@(profile.rightThumbstick.yAxis.value)];
        record[@"buttons"]=@{@"appleA":@(profile.buttonA.value),@"appleB":@(profile.buttonB.value),@"appleX":@(profile.buttonX.value),@"appleY":@(profile.buttonY.value),@"menu":@(profile.buttonMenu.value),@"left_trigger":@(profile.leftTrigger.value),@"right_trigger":@(profile.rightTrigger.value)};
        // Observe the macOS 14+ physical-state owner independently of the
        // legacy extended profile. This neither installs callbacks nor changes
        // queue depth, remapping, permissions or device settings.
        record[@"modern_mapped"]=[self physicalState:[controller.input capture]];
        record[@"modern_unmapped"]=[self physicalState:[controller.input.unmappedInput capture]];
        record[@"adapter_accepted"]=@(accepted);
        if(accepted){const auto tick=slot.input.sample_tick(*_arithmetic,_constants);record[@"native_held"]=@(tick.held);record[@"native_pressed"]=@(tick.pressed);record[@"native_released"]=@(tick.released);record[@"native_left"]=@[@(tick.left.x),@(tick.left.y),@(tick.left.magnitude)];record[@"native_right"]=@[@(tick.right.x),@(tick.right.y),@(tick.right.magnitude)];}
        NSData* data=[NSJSONSerialization dataWithJSONObject:record options:NSJSONWritingSortedKeys error:nil];
        std::string current(static_cast<const char*>(data.bytes),data.length);
        if(current!=slot.previous){slot.previous=current;[self record:record];}
        [text appendFormat:@"Left  x %+0.4f   y %+0.4f   %@\nRight x %+0.4f   y %+0.4f   %@\n",profile.leftThumbstick.xAxis.value,profile.leftThumbstick.yAxis.value,[self bar:profile.leftThumbstick.xAxis.value],profile.rightThumbstick.xAxis.value,profile.rightThumbstick.yAxis.value,[self bar:profile.rightThumbstick.xAxis.value]];
        [text appendFormat:@"Face A:%0.1f B:%0.1f X:%0.1f Y:%0.1f  Menu:%0.1f\nTriggers L:%0.3f R:%0.3f\n\n",profile.buttonA.value,profile.buttonB.value,profile.buttonX.value,profile.buttonY.value,profile.buttonMenu.value,profile.leftTrigger.value,profile.rightTrigger.value];
    }
    if(!visible)[text appendString:@"No GameController profile detected in this foreground app.\n"];
    [text appendString:@"Apple axes are preprocessed; this does not establish original SDL or gameplay parity.\nClose this window when finished. No pairing, rumble, settings, or USB output commands.\n"];
    _details.string=text;
}
- (NSDictionary*)physicalState:(id<GCDevicePhysicalInputState>)state {
    if(!state)return @{@"available":@NO};
    NSMutableDictionary* buttons=[NSMutableDictionary dictionary];
    for(GCInputButtonName name in @[GCInputButtonA,GCInputButtonB,GCInputButtonX,GCInputButtonY,GCInputButtonMenu]) {
        id<GCButtonElement> button=state.buttons[name];
        buttons[name]=button?@{@"pressed":@(button.pressedInput.pressed),@"value":@(button.pressedInput.value)}:[NSNull null];
    }
    return @{@"available":@YES,@"last_event_timestamp":@(state.lastEventTimestamp),@"buttons":buttons};
}
- (NSString*)bar:(float)value {
    NSMutableString* bar=[NSMutableString stringWithString:@"["];
    const int position=static_cast<int>((value+1)*10);
    for(int i=0;i<=20;++i)[bar appendString:i==position?@"●":i==10?@"│":@"─"];
    [bar appendString:@"]"];return bar;
}
- (void)windowWillClose:(NSNotification*)notification{(void)notification;[self cleanup];[NSApp terminate:nil];}
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender{(void)sender;return YES;}
- (void)applicationWillTerminate:(NSNotification*)notification{(void)notification;[self cleanup];}
- (void)cleanup {
    if(_cleaned)return;_cleaned=true;[_timer invalidate];
    for(auto& entry:_slots){entry.second->controller.extendedGamepad.valueChangedHandler=entry.second->previous_handler;entry.second->controller.handlerQueue=entry.second->previous_queue;}
    for(id observer in _observers)[[NSNotificationCenter defaultCenter] removeObserver:observer];
    GCController.shouldMonitorBackgroundEvents=_background;
    [self record:[@{@"event":@"end",@"background_restored":@(_background)} mutableCopy]];_log.close();
}
@end
int main(int argc,char** argv){@autoreleasepool {
    if(argc!=4){return 2;}
    NSApplication* app=[NSApplication sharedApplication];[app setActivationPolicy:NSApplicationActivationPolicyRegular];
    SADXControllerTester* delegate=[SADXControllerTester new];
    if(![delegate configureLog:argv[1] constants:argv[2] mxcsr:static_cast<std::uint32_t>(std::stoul(argv[3],nullptr,0))])return 2;
    app.delegate=delegate;[app run];return 0;
}}

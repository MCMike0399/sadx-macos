#include "platform/mac/mac_input.hpp"
#import <IOKit/hid/IOHIDManager.h>
#import <IOKit/hid/IOHIDKeys.h>
#import <IOKit/hid/IOHIDUsageTables.h>
#import <IOKit/IOKitLib.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>

namespace {
using Clock=std::chrono::steady_clock;
Clock::time_point started;
std::uint64_t hid_events=0;
void emit(NSMutableDictionary* record) {
    record[@"elapsed_ns"]=@(std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now()-started).count());
    NSError* error=nil;NSData* json=[NSJSONSerialization dataWithJSONObject:record options:0 error:&error];
    if(!json){std::cerr<<"JSON serialization failed\n";return;}
    std::cout.write(static_cast<const char*>(json.bytes),json.length);std::cout<<'\n'<<std::flush;
}
id property(IOHIDDeviceRef device,CFStringRef key) {
    CFTypeRef value=IOHIDDeviceGetProperty(device,key);
    if(!value)return [NSNull null];
    const auto type=CFGetTypeID(value);
    if(type!=CFNumberGetTypeID()&&type!=CFStringGetTypeID()&&type!=CFBooleanGetTypeID())return [NSNull null];
    return (__bridge id)value;
}
NSNumber* identity(IOHIDDeviceRef device) {
    std::uint64_t id=0;const auto service=IOHIDDeviceGetService(device);
    if(service&&IORegistryEntryGetRegistryEntryID(service,&id)==KERN_SUCCESS)return @(id);
    return @0;
}
NSMutableDictionary* device_record(IOHIDDeviceRef device) {
    auto record=[NSMutableDictionary dictionary];record[@"registry_id"]=identity(device);
    record[@"product"]=property(device,CFSTR(kIOHIDProductKey));record[@"manufacturer"]=property(device,CFSTR(kIOHIDManufacturerKey));
    record[@"vendor_id"]=property(device,CFSTR(kIOHIDVendorIDKey));record[@"product_id"]=property(device,CFSTR(kIOHIDProductIDKey));
    record[@"transport"]=property(device,CFSTR(kIOHIDTransportKey));record[@"usage_page"]=property(device,CFSTR(kIOHIDPrimaryUsagePageKey));record[@"usage"]=property(device,CFSTR(kIOHIDPrimaryUsageKey));
    record[@"max_input_report"]=property(device,CFSTR(kIOHIDMaxInputReportSizeKey));
    io_name_t name={0};const auto service=IOHIDDeviceGetService(device);
    if(service&&IOObjectGetClass(service,name)==KERN_SUCCESS)record[@"registry_class"]=[NSString stringWithUTF8String:name];else record[@"registry_class"]=[NSNull null];
    io_name_t entry_name={0};
    if(service&&IORegistryEntryGetName(service,entry_name)==KERN_SUCCESS)record[@"registry_name"]=[NSString stringWithUTF8String:entry_name];
    record[@"apple_gc_supports_hid_device"]=@([GCController supportsHIDDevice:device]);
    record[@"source"]=@"IOHID";return record;
}
void matched(void*,IOReturn status,void*,IOHIDDeviceRef device) {
    auto record=device_record(device);record[@"event"]=@"hid_match";record[@"status"]=@(status);emit(record);
}
void removed(void*,IOReturn status,void*,IOHIDDeviceRef device) {
    emit([@{@"event":@"hid_removed",@"registry_id":identity(device),@"status":@(status)} mutableCopy]);
}
void input(void*,IOReturn status,void*,IOHIDValueRef value) {
    const auto element=IOHIDValueGetElement(value);const auto device=IOHIDElementGetDevice(element);
    const auto page=IOHIDElementGetUsagePage(element);
    if(page!=kHIDPage_GenericDesktop&&page!=kHIDPage_Button)return;
    ++hid_events;
    emit([@{@"event":@"hid_value",@"registry_id":identity(device),@"status":@(status),@"usage_page":@(page),
        @"usage":@(IOHIDElementGetUsage(element)),@"cookie":@(IOHIDElementGetCookie(element)),
        @"value":@(IOHIDValueGetIntegerValue(value)),@"logical_min":@(IOHIDElementGetLogicalMin(element)),
        @"logical_max":@(IOHIDElementGetLogicalMax(element)),@"report_id":@(IOHIDElementGetReportID(element))} mutableCopy]);
}
void descriptors(IOHIDDeviceRef device) {
    CFArrayRef elements=IOHIDDeviceCopyMatchingElements(device,nullptr,kIOHIDOptionsTypeNone);
    if(!elements)return;
    NSMutableArray* list=[NSMutableArray array];
    for(CFIndex i=0;i<CFArrayGetCount(elements);++i) {
        auto element=static_cast<IOHIDElementRef>(const_cast<void*>(CFArrayGetValueAtIndex(elements,i)));
        const auto type=IOHIDElementGetType(element);const auto page=IOHIDElementGetUsagePage(element);
        if(type>kIOHIDElementTypeInput_ScanCodes || (page!=kHIDPage_GenericDesktop&&page!=kHIDPage_Button))continue;
        NSMutableDictionary* info=[@{@"cookie":@(IOHIDElementGetCookie(element)),@"type":@(type),@"usage_page":@(page),
            @"usage":@(IOHIDElementGetUsage(element)),@"logical_min":@(IOHIDElementGetLogicalMin(element)),
            @"logical_max":@(IOHIDElementGetLogicalMax(element)),@"report_size":@(IOHIDElementGetReportSize(element)),
            @"report_count":@(IOHIDElementGetReportCount(element)),@"report_id":@(IOHIDElementGetReportID(element))} mutableCopy];
        IOHIDValueRef current=nullptr;const auto read=IOHIDDeviceGetValue(device,element,&current);
        info[@"current_read_status"]=@(read);
        if(read==kIOReturnSuccess&&current)info[@"current_raw_value"]=@(IOHIDValueGetIntegerValue(current));
        [list addObject:info];
    }
    emit([@{@"event":@"hid_elements",@"registry_id":identity(device),@"elements":list} mutableCopy]);CFRelease(elements);
}
struct State {
    sadx::input::InputState input;
    sadx::input::mac::InputAdapter adapter{input};
    std::string previous;
};
NSMutableDictionary* gc_snapshot(GCController* controller,unsigned index,State& state,
    sadx::SseContext& arithmetic,const sadx::InputModAxesConstants& constants) {
    auto record=[NSMutableDictionary dictionary];record[@"source"]=@"GameController";record[@"event"]=@"gc_snapshot";record[@"gc_index"]=@(index);
    record[@"vendor"]=controller.vendorName?:[NSNull null];record[@"category"]=controller.productCategory?:[NSNull null];
    record[@"is_snapshot"]=@(controller.isSnapshot);record[@"extended_gamepad"]=@(controller.extendedGamepad!=nil);
    record[@"physical_profile"]=@(controller.physicalInputProfile!=nil);
    if(controller.physicalInputProfile)record[@"has_remapped_elements"]=@(controller.physicalInputProfile.hasRemappedElements);
    auto profile=controller.extendedGamepad;
    if(profile) {
        record[@"left_raw_apple"]=@[@(profile.leftThumbstick.xAxis.value),@(profile.leftThumbstick.yAxis.value)];
        record[@"right_raw_apple"]=@[@(profile.rightThumbstick.xAxis.value),@(profile.rightThumbstick.yAxis.value)];
        record[@"dpad"]=@[@(profile.dpad.xAxis.value),@(profile.dpad.yAxis.value)];
        record[@"buttons"]=@{@"appleA":@(profile.buttonA.value),@"appleB":@(profile.buttonB.value),@"appleX":@(profile.buttonX.value),@"appleY":@(profile.buttonY.value),
            @"menu":@(profile.buttonMenu.value),@"left_shoulder":@(profile.leftShoulder.value),@"right_shoulder":@(profile.rightShoulder.value),
            @"left_trigger":@(profile.leftTrigger.value),@"right_trigger":@(profile.rightTrigger.value)};
        NSMutableDictionary* names=[NSMutableDictionary dictionary];
        for(NSString* alias in @[GCInputButtonA,GCInputButtonB,GCInputButtonX,GCInputButtonY]) {
            const auto values=[controller.physicalInputProfile mappedPhysicalInputNamesForElementAlias:alias];
            names[alias]=values.allObjects?:@[];
        }
        record[@"physical_alias_names"]=names;
    }
    const bool accepted=state.adapter.controller_sample(controller);record[@"existing_adapter_accepted"]=@(accepted);
    if(accepted) {
        const auto sampled=state.input.sample_tick(arithmetic,constants);
        record[@"native_sample"]=@{@"left":@[@(sampled.left.x),@(sampled.left.y),@(sampled.left.magnitude)],
            @"right":@[@(sampled.right.x),@(sampled.right.y),@(sampled.right.magnitude)],
            @"held":@(sampled.held),@"pressed":@(sampled.pressed),@"released":@(sampled.released),
            @"mxcsr_selected_status":@(sampled.mxcsr_after_sampling),@"analog_source":@"native_normalized_controller_snapshot"};
    }
    return record;
}
}
int main(int argc,char** argv) { @autoreleasepool {
    if(argc!=5&&argc!=6){std::cerr<<"usage: mac-controller-probe SECONDS AXES_CONSTANTS20 MXCSR LABEL [--application]\n";return 2;}
    const double seconds=std::stod(argv[1]);if(seconds<1||seconds>1800)return 2;
    const auto mxcsr=static_cast<std::uint32_t>(std::stoul(argv[3],nullptr,0));
    std::ifstream file(argv[2],std::ios::binary);std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(file),{}};
    const auto constants=sadx::decode_input_mod_axes_constants(bytes.data(),bytes.size());if(!constants){std::cerr<<"requires20-byte pinned axes constants\n";return 2;}
    started=Clock::now();
    const bool application=argc==6&&std::string(argv[5])=="--application";
    if(application){[NSApplication sharedApplication];[NSApp setActivationPolicy:NSApplicationActivationPolicyProhibited];}
    const BOOL prior_background=GCController.shouldMonitorBackgroundEvents;GCController.shouldMonitorBackgroundEvents=YES;
    emit([@{@"event":@"begin",@"label":[NSString stringWithUTF8String:argv[4]],@"seconds":@(seconds),@"application_initialized":@(application),
        @"background_before":@(prior_background),@"background_selected":@YES,@"mxcsr_selected":@(mxcsr),@"diagnostic_poll_hz":@20,
        @"scope":@"passivecontrollerinput;not63FPSgameticks;nohidoutput/pairing/settingswrites"} mutableCopy]);
    auto connect=[[NSNotificationCenter defaultCenter] addObserverForName:GCControllerDidConnectNotification object:nil queue:nil usingBlock:^(NSNotification* note){
        GCController* controller=note.object;emit([@{@"event":@"gc_connected",@"vendor":controller.vendorName?:[NSNull null],@"category":controller.productCategory?:[NSNull null]} mutableCopy]);}];
    auto disconnect=[[NSNotificationCenter defaultCenter] addObserverForName:GCControllerDidDisconnectNotification object:nil queue:nil usingBlock:^(NSNotification* note){
        GCController* controller=note.object;emit([@{@"event":@"gc_disconnected",@"vendor":controller.vendorName?:[NSNull null]} mutableCopy]);}];
    IOHIDManagerRef manager=IOHIDManagerCreate(kCFAllocatorDefault,kIOHIDOptionsTypeNone);
    NSArray* matches=@[@{(__bridge NSString*)CFSTR(kIOHIDVendorIDKey):@0x057e,(__bridge NSString*)CFSTR(kIOHIDProductIDKey):@0x2069},
        @{(__bridge NSString*)CFSTR(kIOHIDVendorIDKey):@0x045e,(__bridge NSString*)CFSTR(kIOHIDProductIDKey):@0x0b12},
        @{(__bridge NSString*)CFSTR(kIOHIDVendorIDKey):@0x045e,(__bridge NSString*)CFSTR(kIOHIDProductIDKey):@0x028e}];
    IOHIDManagerSetDeviceMatchingMultiple(manager,(__bridge CFArrayRef)matches);
    IOHIDManagerRegisterDeviceMatchingCallback(manager,matched,nullptr);IOHIDManagerRegisterDeviceRemovalCallback(manager,removed,nullptr);
    IOHIDManagerRegisterInputValueCallback(manager,input,nullptr);
    IOHIDManagerScheduleWithRunLoop(manager,CFRunLoopGetCurrent(),kCFRunLoopDefaultMode);
    const auto opened=IOHIDManagerOpen(manager,kIOHIDOptionsTypeNone);
    emit([@{@"event":@"hid_open",@"status":@(opened),@"exclusive":@NO,@"persistent_properties":@NO} mutableCopy]);
    CFSetRef initially=IOHIDManagerCopyDevices(manager);
    if(initially){for(id value in [(__bridge NSSet*)initially allObjects])descriptors((__bridge IOHIDDeviceRef)value);CFRelease(initially);}
    emit([@{@"event":@"listener_ready",@"hid_open_success":@(opened==kIOReturnSuccess)} mutableCopy]);
    std::unordered_map<const void*,std::unique_ptr<State>> states;std::unordered_map<const void*,unsigned> indices;unsigned next=0;
    std::uint64_t gc_samples=0;NSUInteger maximum_gc=0;
    sadx::SseContext arithmetic(mxcsr);
    while(std::chrono::duration<double>(Clock::now()-started).count()<seconds) {
        @autoreleasepool {
            for(GCController* controller in GCController.controllers) {
                const auto key=(__bridge const void*)controller;
                if(!states.count(key)){states[key]=std::make_unique<State>();indices[key]=next++;}
                auto record=gc_snapshot(controller,indices[key],*states[key],arithmetic,*constants);
                // Compare only input content, not observer timestamp added byemit.
                NSData* serialized=[NSJSONSerialization dataWithJSONObject:record options:NSJSONWritingSortedKeys error:nil];
                std::string current(static_cast<const char*>(serialized.bytes),serialized.length);
                if(states[key]->previous!=current){states[key]->previous=current;emit(record);++gc_samples;}
            }
            maximum_gc=std::max(maximum_gc,GCController.controllers.count);
            [[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:.05]];
        }
    }
    CFSetRef devices=IOHIDManagerCopyDevices(manager);
    if(devices){NSArray* list=[(__bridge NSSet*)devices allObjects];for(id value in list)descriptors((__bridge IOHIDDeviceRef)value);CFRelease(devices);}
    const auto closed=IOHIDManagerClose(manager,kIOHIDOptionsTypeNone);emit([@{@"event":@"hid_close",@"status":@(closed),@"open_was_successful":@(opened==kIOReturnSuccess)} mutableCopy]);
    IOHIDManagerUnscheduleFromRunLoop(manager,CFRunLoopGetCurrent(),kCFRunLoopDefaultMode);CFRelease(manager);
    [[NSNotificationCenter defaultCenter] removeObserver:connect];[[NSNotificationCenter defaultCenter] removeObserver:disconnect];GCController.shouldMonitorBackgroundEvents=prior_background;
    emit([@{@"event":@"end",@"maximum_gc_count":@(maximum_gc),@"gc_changed_snapshots":@(gc_samples),@"hid_input_values":@(hid_events),@"background_restored":@(prior_background)} mutableCopy]);
    return 0;
}}

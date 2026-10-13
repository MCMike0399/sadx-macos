#import <Foundation/Foundation.h>
#import <IOKit/hidsystem/IOHIDLib.h>
#include <iostream>
int main(){@autoreleasepool {
    const auto access=IOHIDCheckAccess(kIOHIDRequestTypeListenEvent);
    NSData* data=[NSJSONSerialization dataWithJSONObject:@{@"operation":@"IOHIDCheckAccess(ListenEvent)",@"status":@(access),@"scope":@"read-only,noaccessrequestorsettingwrite"} options:0 error:nil];
    std::cout.write(static_cast<const char*>(data.bytes),data.length);std::cout<<'\n';return 0;
}}

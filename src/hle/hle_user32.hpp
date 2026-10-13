#pragma once
#include "guest/x86_guest.hpp"
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>
namespace sadx {
// The window system the guest sees, headless: registered classes, windows (HWND tokens), their longs,
// properties and rectangles, and the messages CreateWindowEx/DestroyWindow send to the guest window
// procedure. Nothing is drawn; the app front end will attach a real window later. Facts Windows would
// take from the desktop (default positions, metrics) are fixed values reported as assumptions.
struct HleWindowClass {
    std::string name;std::uint16_t atom=0;bool unicode=true;
    std::uint32_t style=0,procedure=0,class_extra=0,window_extra=0,instance=0,icon=0,cursor=0,background=0,small_icon=0;
};
struct HleWindow {
    std::uint32_t handle=0,parent=0,owner=0,procedure=0,style=0,ex_style=0,instance=0,menu=0,user_data=0,id=0;
    std::uint16_t atom=0;bool unicode=true,message_only=false,destroyed=false,visible=false,needs_paint=false;
    std::int32_t x=0,y=0,width=0,height=0;std::string text;
    std::vector<std::uint8_t> extra;std::map<std::string,std::uint32_t> properties;
};
struct HleMessage {std::uint32_t hwnd=0,message=0,wparam=0,lparam=0,time=0;std::int32_t x=0,y=0;};
struct HleWindows {
    std::map<std::string,HleWindowClass> classes; // lower-case name
    std::map<std::uint32_t,HleWindow> windows;
    std::vector<HleMessage> queue;                 // posted messages, FIFO
    bool quit_posted=false;std::uint32_t quit_code=0;
    std::uint16_t next_atom=0xc000;std::uint32_t next_window=0x00010010;std::uint32_t focus=0,foreground=0,active=0,capture=0;
    std::map<std::string,std::uint32_t> registered_messages;std::uint32_t next_registered_message=0xc000;
    HleWindow* find(std::uint32_t hwnd){auto i=windows.find(hwnd);return i==windows.end()||i->second.destroyed?nullptr:&i->second;}
};
} // namespace sadx

// USER32 / GDI32 / SHELL32 / SHLWAPI for the guest, headless: classes and windows exist (hle_user32.hpp),
// their procedures receive the messages Windows sends at creation and destruction, and nothing is drawn.
// Functions that would need a real desktop either report a documented fixed answer (an assumption) or stop.
#include "hle/hle_process.hpp"
#include "hle/hle_text.hpp"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <memory>
namespace sadx {
namespace {
std::string lower(std::string s){std::transform(s.begin(),s.end(),s.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});return s;}
constexpr std::uint32_t WM_CREATE=0x1,WM_DESTROY=0x2,WM_NCCREATE=0x81,WM_NCDESTROY=0x82,WM_NCCALCSIZE=0x83,WM_GETMINMAXINFO=0x24;
constexpr std::uint32_t HWND_MESSAGE=0xfffffffdU,CW_USEDEFAULT=0x80000000U,WS_VISIBLE=0x10000000U,WS_CHILD=0x40000000U,WS_CAPTION=0x00c00000U,WS_THICKFRAME=0x00040000U;
// Sends a chain of messages to the guest procedure from inside an HLE function: each step builds its
// message (or ends the chain by returning nullopt) and sees the previous result.
struct Message {std::uint32_t procedure,hwnd,message,wparam,lparam;};
using Step=std::function<std::optional<Message>(X86Guest&,std::optional<std::uint32_t> previous)>;
void send_chain(X86Guest& g,std::uint32_t pops,Step step,std::function<void(X86Guest&)> done){
    auto s=std::make_shared<Step>(std::move(step));auto d=std::make_shared<std::function<void(X86Guest&)>>(std::move(done));
    auto next=std::make_shared<std::function<void(X86Guest&,std::uint32_t,std::optional<std::uint32_t>)>>();
    *next=[s,d,next](X86Guest& gg,std::uint32_t pop,std::optional<std::uint32_t> previous){
        const auto m=(*s)(gg,previous);if(!m){(*d)(gg);return;}
        gg.redirect(m->procedure,{m->hwnd,m->message,m->wparam,m->lparam},pop,[next](X86Guest& g2){(*next)(g2,0,g2.reg(19));});};
    (*next)(g,pops,std::nullopt);
}
}
void register_hle_user32(HleProcess& p){
    auto& r=p.hle();auto* P=&p;auto& W=p.windows;
    const auto arg=[](HleCall& c,unsigned i){return c.argument(i).value_or(0);};
    const auto U=[&](const char* name,std::uint32_t pops,unsigned logged,std::function<std::uint32_t(HleCall&)> body){r.add("USER32.dll",name,{pops,logged,std::move(body)});};
    const auto text=[P](std::uint32_t at,bool wide)->std::optional<std::string>{return wide?P->wide_at(at):P->string_at(at);};
    // --- classes
    const auto register_class=[P,&W,text](HleCall& c,std::uint32_t at,bool wide,bool ex)->std::uint32_t{
        // WNDCLASS[EX]: [cbSize,] style, lpfnWndProc, cbClsExtra, cbWndExtra, hInstance, hIcon, hCursor, hbrBackground, lpszMenuName, lpszClassName[, hIconSm]
        std::uint32_t w[12]={};c.guest.read(at,w,ex?48:40);const auto* f=ex?w+1:w;
        HleWindowClass k;k.unicode=wide;k.style=f[0];k.procedure=f[1];k.class_extra=f[2];k.window_extra=f[3];k.instance=f[4];k.icon=f[5];k.cursor=f[6];k.background=f[7];
        if(ex)k.small_icon=w[11];
        if(f[9]<0x10000){c.guest.stop("RegisterClass with an atom as the class name");return 0;}
        const auto name=text(f[9],wide);if(!name){c.guest.stop("RegisterClass: unreadable class name");return 0;}
        if(W.classes.count(lower(*name))){P->last_error=1410;return 0;} // ERROR_CLASS_ALREADY_EXISTS
        k.name=*name;k.atom=W.next_atom++;W.classes[lower(*name)]=k;return k.atom;};
    U("RegisterClassA",4,1,[register_class,arg](HleCall& c){return register_class(c,arg(c,0),false,false);});
    U("RegisterClassW",4,1,[register_class,arg](HleCall& c){return register_class(c,arg(c,0),true,false);});
    U("RegisterClassExA",4,1,[register_class,arg](HleCall& c){return register_class(c,arg(c,0),false,true);});
    U("RegisterClassExW",4,1,[register_class,arg](HleCall& c){return register_class(c,arg(c,0),true,true);});
    const auto find_class=[&W,text](HleCall& c,std::uint32_t name_or_atom,bool wide)->HleWindowClass*{
        if(name_or_atom<0x10000){for(auto& [n,k]:W.classes)if(k.atom==name_or_atom)return &k;return nullptr;}
        const auto name=text(name_or_atom,wide);if(!name){c.guest.stop("unreadable class name");return nullptr;}
        const auto i=W.classes.find(lower(*name));return i==W.classes.end()?nullptr:&i->second;};
    const auto unregister=[&W,find_class,arg](bool wide){return [&W,find_class,arg,wide](HleCall& c)->std::uint32_t{
        auto* k=find_class(c,arg(c,0),wide);if(!k)return 0;
        for(const auto& [h,w]:W.windows)if(!w.destroyed&&w.atom==k->atom)return 0; // ERROR_CLASS_HAS_WINDOWS
        W.classes.erase(lower(k->name));return 1;};};
    U("UnregisterClassA",8,2,unregister(false));U("UnregisterClassW",8,2,unregister(true));
    U("GetClassInfoExW",12,3,[P,find_class,arg](HleCall& c)->std::uint32_t{
        auto* k=find_class(c,arg(c,1),true);if(!k){P->last_error=1411;return 0;} // ERROR_CLASS_DOES_NOT_EXIST
        const std::uint32_t w[12]={48,k->style,k->procedure,k->class_extra,k->window_extra,k->instance,k->icon,k->cursor,k->background,0,arg(c,1),k->small_icon};
        c.guest.write(arg(c,2),w,sizeof w);return k->atom;});
    // --- windows
    const auto create=[P,&W,text,find_class,arg](bool wide){return [P,&W,text,find_class,arg,wide](HleCall& c)->std::uint32_t{
        // (ex_style, class, name, style, x, y, width, height, parent, menu, instance, param)
        auto* k=find_class(c,arg(c,1),wide);if(!k){P->last_error=1407;return 0;} // ERROR_CANNOT_FIND_WND_CLASS
        HleWindow w;w.handle=W.next_window;W.next_window+=4;w.atom=k->atom;w.procedure=k->procedure;w.unicode=wide;
        w.ex_style=arg(c,0);w.style=arg(c,3);w.parent=arg(c,8)==HWND_MESSAGE?0:arg(c,8);w.message_only=arg(c,8)==HWND_MESSAGE;w.menu=arg(c,9);w.instance=arg(c,10);
        if(w.style&WS_CHILD)w.id=arg(c,9);
        if(arg(c,2))w.text=text(arg(c,2),wide).value_or("");
        const auto coordinate=[&](unsigned i){return static_cast<std::int32_t>(arg(c,i));};
        w.x=arg(c,4)==CW_USEDEFAULT?0:coordinate(4);w.y=arg(c,4)==CW_USEDEFAULT?0:coordinate(5);
        w.width=arg(c,6)==CW_USEDEFAULT?0:coordinate(6);w.height=arg(c,6)==CW_USEDEFAULT?0:coordinate(7);
        if(arg(c,4)==CW_USEDEFAULT||arg(c,6)==CW_USEDEFAULT)P->assume("CreateWindowEx CW_USEDEFAULT: position 0,0 and size 0x0 (the desktop's cascade was not captured)");
        w.extra.assign(k->window_extra,0);
        if(w.style&WS_VISIBLE){c.guest.stop("CreateWindowEx of a visible window: the WM_SHOWWINDOW/WM_WINDOWPOS* sequence is not modelled yet");return 0;}
        const auto hwnd=w.handle;W.windows[hwnd]=w;
        // CREATESTRUCT for WM_NCCREATE/WM_CREATE, a RECT for WM_NCCALCSIZE, MINMAXINFO for WM_GETMINMAXINFO.
        const auto cs=P->memory().allocate(48+16+40).value_or(0);
        const std::uint32_t s[12]={arg(c,11),w.instance,w.menu,arg(c,8),static_cast<std::uint32_t>(w.height),static_cast<std::uint32_t>(w.width),static_cast<std::uint32_t>(w.y),static_cast<std::uint32_t>(w.x),w.style,arg(c,2),arg(c,1),w.ex_style};
        c.guest.write(cs,s,48);const std::int32_t rect[4]={w.x,w.y,w.x+w.width,w.y+w.height};c.guest.write(cs+48,rect,16);
        const bool top_level=!(w.style&WS_CHILD)&&!w.message_only&&(w.style&(WS_CAPTION|WS_THICKFRAME));
        auto stage=std::make_shared<int>(top_level?0:1);auto failed=std::make_shared<bool>(false);
        send_chain(c.guest,48,[&W,hwnd,cs,stage,failed](X86Guest&,std::optional<std::uint32_t> previous)->std::optional<Message>{
            auto* win=W.find(hwnd);if(!win)return std::nullopt;
            switch((*stage)++){
            case 0:return Message{win->procedure,hwnd,WM_GETMINMAXINFO,0,cs+64};
            case 1:return Message{win->procedure,hwnd,WM_NCCREATE,0,cs};
            case 2:if(!previous||!*previous){*failed=true;return std::nullopt;}return Message{win->procedure,hwnd,WM_NCCALCSIZE,0,cs+48};
            case 3:return Message{win->procedure,hwnd,WM_CREATE,0,cs};
            default:if(previous&&*previous==0xffffffffU)*failed=true;return std::nullopt;}},
            [P,&W,hwnd,cs,failed](X86Guest& g){P->memory().free(cs);if(*failed){W.windows[hwnd].destroyed=true;g.set_eax(0);}else g.set_eax(hwnd);});
        return hwnd;};};
    U("CreateWindowExA",48,12,create(false));U("CreateWindowExW",48,12,create(true));
    const auto def_window_proc=[P,&W,arg](bool wide){return [P,&W,arg,wide](HleCall& c)->std::uint32_t{
        const auto message=arg(c,1);auto* w=W.find(arg(c,0));
        switch(message){
        case WM_NCCREATE:{if(w){std::uint32_t name=0;c.guest.read(arg(c,3)+36,&name,4);if(name)w->text=(wide?P->wide_at(name):P->string_at(name)).value_or("");}return 1;}
        case WM_CREATE:case WM_DESTROY:case WM_NCDESTROY:case WM_GETMINMAXINFO:case WM_NCCALCSIZE:return 0;
        case 0x0018:case 0x0046:case 0x0047:case 0x001c:case 0x0006:case 0x0086:case 0x0007:case 0x0008:case 0x0005:case 0x0003:case 0x00ff:case 0x0219:return 0;
            // WM_SHOWWINDOW, WM_WINDOWPOSCHANGING/CHANGED, WM_ACTIVATEAPP, WM_ACTIVATE, WM_NCACTIVATE (handled as 0), WM_SETFOCUS, WM_KILLFOCUS, WM_SIZE, WM_MOVE, WM_INPUT, WM_DEVICECHANGE
        default:c.guest.stop("DefWindowProc message "+std::to_string(message)+" is not modelled");return 0;}};};
    U("DefWindowProcA",16,4,def_window_proc(false));U("DefWindowProcW",16,4,def_window_proc(true));
    U("CallWindowProcW",20,5,[arg](HleCall& c)->std::uint32_t{
        c.guest.redirect(arg(c,0),{arg(c,1),arg(c,2),arg(c,3),arg(c,4)},20,[](X86Guest&){});return 0;});
    U("DestroyWindow",4,1,[P,&W,arg](HleCall& c)->std::uint32_t{
        const auto hwnd=arg(c,0);if(!W.find(hwnd)){P->last_error=1400;return 0;} // ERROR_INVALID_WINDOW_HANDLE
        auto stage=std::make_shared<int>(0);
        send_chain(c.guest,4,[&W,hwnd,stage](X86Guest&,std::optional<std::uint32_t>)->std::optional<Message>{
            auto* w=W.find(hwnd);if(!w)return std::nullopt;
            switch((*stage)++){case 0:return Message{w->procedure,hwnd,WM_DESTROY,0,0};case 1:return Message{w->procedure,hwnd,WM_NCDESTROY,0,0};default:return std::nullopt;}},
            [&W,hwnd](X86Guest& g){W.windows[hwnd].destroyed=true;g.set_eax(1);});
        return 1;});
    const auto get_long=[P,&W,arg](HleCall& c)->std::uint32_t{
        auto* w=W.find(arg(c,0));if(!w){P->last_error=1400;return 0;}
        const auto index=static_cast<std::int32_t>(arg(c,1));
        switch(index){case -4:return w->procedure;case -6:return w->instance;case -8:return w->owner;case -12:return w->id;case -16:return w->style;case -20:return w->ex_style;case -21:return w->user_data;default:break;}
        if(index<0||static_cast<std::size_t>(index)+4>w->extra.size()){P->last_error=1413;return 0;} // ERROR_INVALID_INDEX
        std::uint32_t v;std::memcpy(&v,&w->extra[static_cast<std::size_t>(index)],4);return v;};
    const auto set_long=[P,&W,arg](HleCall& c)->std::uint32_t{
        auto* w=W.find(arg(c,0));if(!w){P->last_error=1400;return 0;}
        const auto index=static_cast<std::int32_t>(arg(c,1));const auto v=arg(c,2);std::uint32_t old=0;
        switch(index){case -4:old=w->procedure;w->procedure=v;return old;case -12:old=w->id;w->id=v;return old;case -21:old=w->user_data;w->user_data=v;return old;
            case -16:case -20:c.guest.stop("SetWindowLong of a style: WM_STYLECHANGING/CHANGED are not modelled");return 0;default:break;}
        if(index<0||static_cast<std::size_t>(index)+4>w->extra.size()){P->last_error=1413;return 0;}
        std::memcpy(&old,&w->extra[static_cast<std::size_t>(index)],4);std::memcpy(&w->extra[static_cast<std::size_t>(index)],&v,4);return old;};
    U("GetWindowLongA",8,2,get_long);U("GetWindowLongW",8,2,get_long);U("SetWindowLongA",12,3,set_long);U("SetWindowLongW",12,3,set_long);
    const auto property_name=[P](HleCall& c,std::uint32_t at)->std::optional<std::string>{if(at<0x10000)return "#"+std::to_string(at);const auto s=P->wide_at(at);if(!s)c.guest.stop("unreadable property name");return s;};
    U("SetPropW",12,3,[&W,arg,property_name](HleCall& c)->std::uint32_t{auto* w=W.find(arg(c,0));const auto n=property_name(c,arg(c,1));if(!w||!n)return 0;w->properties[*n]=arg(c,2);return 1;});
    U("GetPropW",8,2,[&W,arg,property_name](HleCall& c)->std::uint32_t{auto* w=W.find(arg(c,0));const auto n=property_name(c,arg(c,1));if(!w||!n)return 0;const auto i=w->properties.find(*n);return i==w->properties.end()?0:i->second;});
    U("RemovePropW",8,2,[&W,arg,property_name](HleCall& c)->std::uint32_t{auto* w=W.find(arg(c,0));const auto n=property_name(c,arg(c,1));if(!w||!n)return 0;const auto i=w->properties.find(*n);if(i==w->properties.end())return 0;const auto v=i->second;w->properties.erase(i);return v;});
    U("RegisterWindowMessageA",4,1,[P,&W,arg](HleCall& c)->std::uint32_t{
        const auto s=P->string_at(arg(c,0));if(!s)return 0;auto& m=W.registered_messages[lower(*s)];if(!m)m=W.next_registered_message++;return m;});
    // Devices: the guest machine has no game controllers or HID devices (input reaches the game through the
    // host later); enumerations are empty and per-device functions stop because no device can exist.
    P->assume("no game controllers or HID devices are attached to the guest machine (SetupAPI/HID enumerations are empty)");
    for(const char* name:{"HidD_GetAttributes","HidD_GetSerialNumberString","HidD_GetManufacturerString","HidD_GetProductString","HidD_SetFeature","HidD_GetFeature",
                          "HidD_GetInputReport","HidD_GetIndexedString","HidD_GetPreparsedData","HidD_FreePreparsedData","HidP_GetCaps","HidD_SetNumInputBuffers",
                          "HidD_SetOutputReport","HidP_GetButtonCaps","HidP_GetValueCaps","HidP_MaxDataListLength","HidP_GetData"})
        r.add("hid.dll",name,{0,0,[name](HleCall& c)->std::uint32_t{c.guest.stop(std::string("hid.dll!")+name+" on a device the guest machine does not have");return 0;}});
    r.add("hid.dll","HidD_GetHidGuid",{4,1,[](HleCall& c)->std::uint32_t{
        const std::uint8_t guid[16]={0xb2,0x55,0x1e,0x4d,0x6f,0xf1,0xcf,0x11,0x88,0xcb,0x00,0x11,0x11,0x00,0x00,0x30};c.guest.write(c.argument(0).value_or(0),guid,16);return 0;}});
    r.add("SETUPAPI.dll","SetupDiGetClassDevsA",{16,4,[P](HleCall&)->std::uint32_t{return P->new_handle();}});
    r.add("SETUPAPI.dll","SetupDiGetClassDevsW",{16,4,[P](HleCall&)->std::uint32_t{return P->new_handle();}});
    for(const char* name:{"SetupDiEnumDeviceInterfaces","SetupDiEnumDeviceInfo"}){
        const std::uint32_t pops=std::string(name)=="SetupDiEnumDeviceInterfaces"?20:12;
        r.add("SETUPAPI.dll",name,{pops,3,[P](HleCall&)->std::uint32_t{P->last_error=259;return 0;}});} // ERROR_NO_MORE_ITEMS
    r.add("SETUPAPI.dll","SetupDiDestroyDeviceInfoList",{4,1,[](HleCall&)->std::uint32_t{return 1;}});
    // Device-arrival notifications and raw input: registered, never delivered (no devices arrive).
    U("RegisterDeviceNotificationW",12,3,[P](HleCall&)->std::uint32_t{return P->new_handle();});
    U("UnregisterDeviceNotification",4,1,[](HleCall&)->std::uint32_t{return 1;});
    U("RegisterRawInputDevices",12,3,[](HleCall&)->std::uint32_t{return 1;});
    U("GetRawInputDeviceList",12,3,[arg](HleCall& c)->std::uint32_t{if(arg(c,1))c.guest.set_u32(arg(c,1),0);return 0;});
    // COM apartment calls (one thread: initialisation always succeeds); object creation is decided per CLSID.
    r.add("ole32.dll","CoInitializeEx",{8,2,[](HleCall&)->std::uint32_t{return 0;}});
    r.add("ole32.dll","CoInitialize",{4,1,[](HleCall&)->std::uint32_t{return 0;}});
    r.add("ole32.dll","CoUninitialize",{0,0,[](HleCall&)->std::uint32_t{return 0;}});
    r.add("ole32.dll","CoCreateInstance",{20,5,[](HleCall& c)->std::uint32_t{
        std::uint32_t g[4]={};c.guest.read(c.argument(0).value_or(0),g,16);char b[64];std::snprintf(b,sizeof b,"%08x-%08x-%08x-%08x",g[0],g[1],g[2],g[3]);
        c.guest.stop(std::string("CoCreateInstance of CLSID ")+b+" is not modelled");return 0x80004005U;}});
    // XInput (1_4, 1_3, 9_1_0): no controller is connected to the guest machine.
    for(const char* dll:{"XInput1_4.dll","XInput1_3.dll","XInput9_1_0.dll"}){
        const auto none=[](HleCall&)->std::uint32_t{return 1167;}; // ERROR_DEVICE_NOT_CONNECTED
        r.add(dll,"XInputGetState",{8,2,none});r.add(dll,"XInputSetState",{8,2,none});r.add(dll,"XInputGetCapabilities",{12,3,none});
        r.add(dll,"XInputGetBatteryInformation",{12,3,none});r.add(dll,"XInputGetKeystroke",{12,3,none});
        r.add(dll,"#100",{8,2,none});                 // XInputGetStateEx
        r.add(dll,"#108",{16,4,none});                // XInputGetCapabilitiesEx
        r.add(dll,"XInputEnable",{4,1,[](HleCall&)->std::uint32_t{return 0;}});}
    r.add("cfgmgr32.dll","CM_Register_Notification",{16,4,[P](HleCall& c)->std::uint32_t{c.guest.set_u32(c.argument(3).value_or(0),P->new_handle());return 0;}}); // CR_SUCCESS
    r.add("cfgmgr32.dll","CM_Unregister_Notification",{4,1,[](HleCall&)->std::uint32_t{return 0;}});
    // WinRT (combase): apartments initialise; activation factories (Windows.Gaming.Input…) are not provided.
    r.add("combase.dll","RoInitialize",{4,1,[](HleCall&)->std::uint32_t{return 0;}});
    r.add("combase.dll","RoUninitialize",{0,0,[](HleCall&)->std::uint32_t{return 0;}});
    r.add("combase.dll","RoGetActivationFactory",{12,3,[P](HleCall&)->std::uint32_t{
        P->assume("WinRT activation factories (Windows.Gaming.Input) are not provided: RoGetActivationFactory returns REGDB_E_CLASSNOTREG; with no controllers attached the game sees no difference");
        return 0x80040154U;}});
    r.add("combase.dll","WindowsCreateStringReference",{16,4,[](HleCall& c)->std::uint32_t{
        // HSTRING_HEADER (20 bytes): flags 1 (reference), length, padding, the buffer pointer; the HSTRING is the header address.
        const auto header=c.argument(2).value_or(0);const std::uint32_t h[5]={1,c.argument(1).value_or(0),0,0,c.argument(0).value_or(0)};
        c.guest.write(header,h,20);c.guest.set_u32(c.argument(3).value_or(0),header);return 0;}});
    r.add("combase.dll","CoIncrementMTAUsage",{4,1,[P](HleCall& c)->std::uint32_t{c.guest.set_u32(c.argument(0).value_or(0),P->new_handle());return 0;}});
    r.add("combase.dll","CoDecrementMTAUsage",{4,1,[](HleCall&)->std::uint32_t{return 0;}});
    r.add("combase.dll","WindowsDeleteString",{4,1,[](HleCall&)->std::uint32_t{return 0;}});
    // SHELL32!CommandLineToArgvW: argv[0] up to the first space or the closing quote; the rest with the
    // documented quote/backslash rules. One LocalAlloc block: the pointer array, then the strings.
    r.add("SHELL32.dll","CommandLineToArgvW",{8,2,[P](HleCall& c)->std::uint32_t{
        const auto line=P->wide_at(c.argument(0).value_or(0));if(!line){c.guest.stop("CommandLineToArgvW: unreadable or non-ASCII command line");return 0;}
        std::vector<std::string> args;std::size_t i=0;const auto& s=*line;
        {std::string a;if(i<s.size()&&s[i]=='"'){++i;while(i<s.size()&&s[i]!='"')a+=s[i++];if(i<s.size())++i;}else while(i<s.size()&&s[i]!=' '&&s[i]!='\t')a+=s[i++];args.push_back(a);}
        while(true){while(i<s.size()&&(s[i]==' '||s[i]=='\t'))++i;if(i>=s.size())break;std::string a;bool quoted=false;
            while(i<s.size()&&(quoted||(s[i]!=' '&&s[i]!='\t'))){
                std::size_t slashes=0;while(i<s.size()&&s[i]=='\\'){++slashes;++i;}
                if(i<s.size()&&s[i]=='"'){a.append(slashes/2,'\\');if(slashes%2){a+='"';++i;}else{if(quoted&&i+1<s.size()&&s[i+1]=='"'){a+='"';++i;}else quoted=!quoted;++i;}}
                else{a.append(slashes,'\\');if(i<s.size()&&(quoted||(s[i]!=' '&&s[i]!='\t')))a+=s[i++];}}
            args.push_back(a);}
        std::uint32_t size=static_cast<std::uint32_t>(4*(args.size()+1));for(const auto& a:args)size+=static_cast<std::uint32_t>(2*(a.size()+1));
        const auto block=P->memory().allocate(size).value_or(0);if(!block)return 0;
        std::uint32_t at=block+static_cast<std::uint32_t>(4*(args.size()+1));
        for(std::size_t k=0;k<args.size();++k){c.guest.set_u32(block+4*static_cast<std::uint32_t>(k),at);P->put_wide(at,args[k]);at+=static_cast<std::uint32_t>(2*(args[k].size()+1));}
        c.guest.set_u32(block+4*static_cast<std::uint32_t>(args.size()),0);c.guest.set_u32(c.argument(1).value_or(0),static_cast<std::uint32_t>(args.size()));return block;}});
    U("SetProcessDPIAware",0,0,[P](HleCall&)->std::uint32_t{P->assume("SetProcessDPIAware succeeds and has no effect (no host DPI scaling is modelled)");return 1;});
    // SHLWAPI path helpers on guest strings (pointers into the argument).
    r.add("SHLWAPI.dll","PathFindFileNameA",{4,1,[P](HleCall& c)->std::uint32_t{
        const auto at=c.argument(0).value_or(0);const auto s=P->string_at(at);if(!s)return at;
        std::size_t start=0;for(std::size_t i=0;i<s->size();++i)if(((*s)[i]=='\\'||(*s)[i]=='/'||(*s)[i]==':')&&i+1<s->size())start=i+1;
        return at+static_cast<std::uint32_t>(start);}});
    r.add("SHLWAPI.dll","PathFindExtensionA",{4,1,[P](HleCall& c)->std::uint32_t{
        const auto at=c.argument(0).value_or(0);const auto s=P->string_at(at);if(!s)return at;
        std::size_t dot=s->size();for(std::size_t i=0;i<s->size();++i){const char ch=(*s)[i];if(ch=='.')dot=i;else if(ch=='\\'||ch=='/'||ch==' ')dot=s->size();}
        return at+static_cast<std::uint32_t>(dot);}});
}
} // namespace sadx

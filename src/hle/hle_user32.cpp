// USER32 / GDI32 / SHELL32 / SHLWAPI for the guest, headless: classes and windows exist (hle_user32.hpp),
// their procedures receive the messages Windows sends at creation and destruction, and nothing is drawn.
// Functions that would need a real desktop either report a documented fixed answer (an assumption) or stop.
#include "hle/hle_process.hpp"
#include "hle/hle_text.hpp"
#include "hle/hle_printf.hpp"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <array>
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
namespace {
// The desktop of the modelled machine: one 1920x1080 monitor at 96 DPI, Windows 10 metrics.
constexpr std::int32_t screen_width=1920,screen_height=1080,caption=23,frame=4,padded=4,fixed_frame=3;
struct Size {std::int32_t w,h;};
struct Point {std::int32_t x,y;};
// Non-client insets of a window (left, top, right, bottom), the inverse of AdjustWindowRectEx.
std::array<std::int32_t,4> insets(std::uint32_t style,std::uint32_t ex_style){
    std::int32_t b=0;
    if(style&WS_THICKFRAME)b=frame+padded;else if((style&WS_CAPTION)||(style&0x00400000U)||(ex_style&0x1))b=fixed_frame;else if(style&0x00800000U)b=1;
    if(ex_style&0x200)b+=2;if(ex_style&0x20000)b+=1; // WS_EX_CLIENTEDGE, WS_EX_STATICEDGE
    return {b,b+((style&WS_CAPTION)==WS_CAPTION?caption:0),b,b};}
Size client_size(const HleWindow& w){const auto i=insets(w.style,w.ex_style);return {std::max(0,w.width-i[0]-i[2]),std::max(0,w.height-i[1]-i[3])};}
Point client_origin(const HleWindow& w){const auto i=insets(w.style,w.ex_style);return {w.x+i[0],w.y+i[1]};}
std::uint32_t pack_size(Size s){return (static_cast<std::uint32_t>(s.h&0xffff)<<16)|static_cast<std::uint32_t>(s.w&0xffff);}
std::uint32_t pack_point(Point p){return (static_cast<std::uint32_t>(p.y&0xffff)<<16)|static_cast<std::uint32_t>(p.x&0xffff);}
}
namespace {
void register_hle_user32_desktop(HleProcess& p);
}
void register_hle_user32(HleProcess& p){
    register_hle_user32_desktop(p);
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
    // ShowWindow's messages (Windows' order as Wine's message tests record it; NCPAINT/ERASEBKGND/IME omitted):
    // WM_SHOWWINDOW, WM_WINDOWPOSCHANGING, for a top-level window becoming active WM_ACTIVATEAPP (when the
    // application was inactive), WM_NCACTIVATE, WM_ACTIVATE (DefWindowProc then sets the focus), and
    // WM_WINDOWPOSCHANGED (SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE). A child gets WM_SHOWWINDOW and the pair.
    auto show=std::make_shared<std::function<void(X86Guest&,std::uint32_t,bool,std::function<void(X86Guest&)>)>>();
    auto windowpos=std::make_shared<std::uint32_t>(0);
    *show=[P,&W,windowpos](X86Guest& g,std::uint32_t hwnd,bool chained,std::function<void(X86Guest&)> done){
        P->assume("window message order (creation, ShowWindow, activation, SetWindowPos) follows Windows' documented sequence as Wine's message tests record it; WM_NCPAINT/WM_ERASEBKGND/IME messages are not sent; not traced on the reference");
        if(!*windowpos)*windowpos=P->memory().allocate(28).value_or(0);
        auto* w=W.find(hwnd);if(!w){done(g);return;}
        const bool child=(w->style&WS_CHILD)!=0;const bool activate=!child&&W.active!=hwnd;const bool app_was_inactive=W.active==0;
        w->visible=true;w->style|=WS_VISIBLE;w->needs_paint=true;
        const std::uint32_t pos[7]={hwnd,0,static_cast<std::uint32_t>(w->x),static_cast<std::uint32_t>(w->y),static_cast<std::uint32_t>(w->width),static_cast<std::uint32_t>(w->height),0x43}; // SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE
        g.write(*windowpos,pos,28);
        auto stage=std::make_shared<int>(0);const auto wp=*windowpos;
        send_chain(g,chained?0:0,[&W,hwnd,stage,child,activate,app_was_inactive,wp](X86Guest&,std::optional<std::uint32_t>)->std::optional<Message>{
            auto* win=W.find(hwnd);if(!win)return std::nullopt;
            for(;;)switch((*stage)++){
            case 0:return Message{win->procedure,hwnd,0x0018,1,0};          // WM_SHOWWINDOW TRUE
            case 1:return Message{win->procedure,hwnd,0x0046,0,wp};         // WM_WINDOWPOSCHANGING
            case 2:if(child||!activate||!app_was_inactive)continue;W.active=hwnd;W.foreground=hwnd;return Message{win->procedure,hwnd,0x001c,1,0}; // WM_ACTIVATEAPP
            case 3:if(child||!activate)continue;W.active=hwnd;W.foreground=hwnd;return Message{win->procedure,hwnd,0x0086,1,0}; // WM_NCACTIVATE
            case 4:if(child||!activate)continue;return Message{win->procedure,hwnd,0x0006,1,0}; // WM_ACTIVATE WA_ACTIVE
            case 5:return Message{win->procedure,hwnd,0x0047,0,wp};         // WM_WINDOWPOSCHANGED
            default:return std::nullopt;}},
            [done](X86Guest& gg){done(gg);});
        (void)chained;};
    const auto create=[P,&W,text,find_class,arg,show](bool wide){return [P,&W,text,find_class,arg,wide,show](HleCall& c)->std::uint32_t{
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
            case 4:if(previous&&*previous==0xffffffffU){*failed=true;return std::nullopt;}
                return Message{win->procedure,hwnd,0x0005,0,pack_size(client_size(*win))}; // WM_SIZE, SIZE_RESTORED
            case 5:return Message{win->procedure,hwnd,0x0003,0,pack_point(client_origin(*win))}; // WM_MOVE
            default:return std::nullopt;}},
            [P,&W,hwnd,cs,failed,show](X86Guest& g){P->memory().free(cs);
                if(*failed){W.windows[hwnd].destroyed=true;g.set_eax(0);return;}
                g.set_eax(hwnd);
                if(W.windows[hwnd].style&WS_VISIBLE)(*show)(g,hwnd,true,[hwnd](X86Guest& g2){g2.set_eax(hwnd);});});
        return hwnd;};};
    U("CreateWindowExA",48,12,create(false));U("CreateWindowExW",48,12,create(true));
    const auto def_window_proc=[P,&W,arg](bool wide){return [P,&W,arg,wide](HleCall& c)->std::uint32_t{
        const auto hwnd=arg(c,0),message=arg(c,1),wparam=arg(c,2),lparam=arg(c,3);auto* w=W.find(hwnd);
        switch(message){
        case WM_NCCREATE:{if(w){std::uint32_t name=0;c.guest.read(lparam+36,&name,4);if(name)w->text=(wide?P->wide_at(name):P->string_at(name)).value_or("");}return 1;}
        case 0x0006:{ // WM_ACTIVATE: DefWindowProc gives the window the focus when it becomes active (and is not minimised)
            if((wparam&0xffff)!=0&&W.focus!=hwnd){const auto previous=W.focus;W.focus=hwnd;auto stage=std::make_shared<int>(0);
                send_chain(c.guest,16,[&W,hwnd,previous,stage](X86Guest&,std::optional<std::uint32_t>)->std::optional<Message>{
                    for(;;)switch((*stage)++){case 0:if(!previous||!W.find(previous))continue;return Message{W.find(previous)->procedure,previous,0x0008,hwnd,0}; // WM_KILLFOCUS
                        case 1:if(!W.find(hwnd))return std::nullopt;return Message{W.find(hwnd)->procedure,hwnd,0x0007,previous,0}; // WM_SETFOCUS
                        default:return std::nullopt;}},[](X86Guest& g){g.set_eax(0);});}
            return 0;}
        case 0x0047:{ // WM_WINDOWPOSCHANGED: WM_SIZE / WM_MOVE unless SWP_NOSIZE / SWP_NOMOVE
            std::uint32_t flags=0;c.guest.read(lparam+24,&flags,4);if(!w||((flags&1)&&(flags&2)))return 0;
            auto stage=std::make_shared<int>(0);
            send_chain(c.guest,16,[&W,hwnd,flags,stage](X86Guest&,std::optional<std::uint32_t>)->std::optional<Message>{
                auto* win=W.find(hwnd);if(!win)return std::nullopt;
                for(;;)switch((*stage)++){case 0:if(flags&1)continue;return Message{win->procedure,hwnd,0x0005,0,pack_size(client_size(*win))};
                    case 1:if(flags&2)continue;return Message{win->procedure,hwnd,0x0003,0,pack_point(client_origin(*win))};default:return std::nullopt;}},[](X86Guest& g){g.set_eax(0);});
            return 0;}
        case 0x000f:if(w)w->needs_paint=false;return 0;                  // WM_PAINT: BeginPaint/EndPaint validate the window
        case 0x0014:return 1;                                            // WM_ERASEBKGND: the class brush fills the background (nothing is drawn headless)
        case 0x0086:return 1;                                            // WM_NCACTIVATE: TRUE lets the activation proceed
        case 0x0010:c.guest.stop("DefWindowProc WM_CLOSE (DestroyWindow from inside a message) is not modelled");return 0;
        case 0x0020:return 0;                                            // WM_SETCURSOR: FALSE (the class cursor; nothing to show)
        case 0x0100:case 0x0101:case 0x0102:return 0;                   // WM_KEYDOWN, WM_KEYUP, WM_CHAR: nothing (only WM_SYSKEY* have default actions)
        case WM_CREATE:case WM_DESTROY:case WM_NCDESTROY:case WM_GETMINMAXINFO:case WM_NCCALCSIZE:return 0;
        case 0x0018:case 0x0046:case 0x001c:case 0x0007:case 0x0008:case 0x0005:case 0x0003:case 0x00ff:case 0x0219:case 0x0022:return 0;
            // WM_SHOWWINDOW, WM_WINDOWPOSCHANGING, WM_ACTIVATEAPP, WM_SETFOCUS, WM_KILLFOCUS, WM_SIZE, WM_MOVE, WM_INPUT, WM_DEVICECHANGE, WM_CHILDACTIVATE
        default:c.guest.stop("DefWindowProc message "+std::to_string(message)+" is not modelled");return 0;}};};
    U("DefWindowProcA",16,4,def_window_proc(false));U("DefWindowProcW",16,4,def_window_proc(true));
    const auto call_window_proc=[arg](HleCall& c)->std::uint32_t{
        if(!arg(c,0))return 0; // a NULL procedure: CallWindowProc returns 0 without calling anything
        c.guest.redirect(arg(c,0),{arg(c,1),arg(c,2),arg(c,3),arg(c,4)},20,[](X86Guest&){});return 0;};
    U("CallWindowProcW",20,5,call_window_proc);U("CallWindowProcA",20,5,call_window_proc);
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
    P->assume("SetupAPI/HID enumerations are empty; XInput device state is supplied separately by host samples");
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
    r.add("cfgmgr32.dll","CM_Register_Notification",{16,4,[P](HleCall& c)->std::uint32_t{c.guest.set_u32(c.argument(3).value_or(0),P->new_handle());return 0;}}); // CR_SUCCESS
    r.add("cfgmgr32.dll","CM_Unregister_Notification",{4,1,[](HleCall&)->std::uint32_t{return 0;}});
    // WinRT (combase): apartments initialise; activation factories (Windows.Gaming.Input…) are not provided.
    r.add("combase.dll","RoInitialize",{4,1,[](HleCall&)->std::uint32_t{return 0;}});
    r.add("combase.dll","RoUninitialize",{0,0,[](HleCall&)->std::uint32_t{return 0;}});
    r.add("combase.dll","RoGetActivationFactory",{12,3,[P](HleCall&)->std::uint32_t{
        P->assume("WinRT activation factories (Windows.Gaming.Input) are not provided: RoGetActivationFactory returns REGDB_E_CLASSNOTREG; the synthetic XInput path is modelled separately");
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
    r.add("IMM32.DLL","ImmDisableIME",{4,1,[](HleCall&)->std::uint32_t{return 1;}}); // no IME exists headless; the call changes no game state
    // wsprintfA/W (cdecl varargs): the printf engine without floating point, at most 1024 characters.
    const auto wsprintf=[](bool wide){return [wide](HleCall& c)->std::uint32_t{
        const auto out=c.argument(0).value_or(0),format=c.argument(1).value_or(0);const auto unit=wide?2U:1U;
        std::vector<std::uint16_t> f;for(std::uint32_t i=0;;++i){std::uint16_t ch=0;if(!c.guest.read(format+unit*i,&ch,unit)){c.guest.stop("wsprintf: unreadable format");return 0;}if(!ch)break;f.push_back(wide?ch:cp1252_to_utf16(static_cast<std::uint8_t>(ch)));}
        for(const auto ch:f)(void)ch;
        std::string error;const auto text=format_guest(c.guest,f,c.guest.esp()+12,wide,&error);if(!text){c.guest.stop("wsprintf: "+error);return 0;}
        const auto n=static_cast<std::uint32_t>(std::min<std::size_t>(text->size(),1023));
        for(std::uint32_t i=0;i<n;++i){if(wide)c.guest.write(out+2*i,&(*text)[i],2);else{const auto b=utf16_to_cp1252((*text)[i]).value_or(static_cast<std::uint8_t>('?'));c.guest.write(out+i,&b,1);}}
        const std::uint16_t z=0;c.guest.write(out+unit*n,&z,unit);return n;};};
    U("wsprintfA",0,2,wsprintf(false));U("wsprintfW",0,2,wsprintf(true));
    // --- showing, focus, position, painting, messages
    U("ShowWindow",8,2,[&W,arg,show](HleCall& c)->std::uint32_t{
        auto* w=W.find(arg(c,0));if(!w)return 0;const bool was=w->visible;const auto cmd=arg(c,1);
        if(cmd==0){c.guest.stop("ShowWindow(SW_HIDE) is not modelled");return 0;}
        if(cmd!=1&&cmd!=5&&cmd!=10&&cmd!=8&&cmd!=9){c.guest.stop("ShowWindow command "+std::to_string(cmd)+" (minimise/maximise) is not modelled");return 0;}
        if(was)return 1; // already visible: nothing is sent, the previous state (visible) is returned
        // The first message is sent from this HLE frame (8 argument bytes); the result is the previous visibility (0).
        (*show)(c.guest,arg(c,0),false,[](X86Guest& g){g.set_eax(0);});
        return 0;});
    U("UpdateWindow",4,1,[&W,arg](HleCall& c)->std::uint32_t{
        auto* w=W.find(arg(c,0));if(!w)return 0;if(!w->needs_paint)return 1;
        const auto hwnd=arg(c,0);auto stage=std::make_shared<int>(0);
        send_chain(c.guest,4,[&W,hwnd,stage](X86Guest&,std::optional<std::uint32_t>)->std::optional<Message>{
            auto* win=W.find(hwnd);if(!win||(*stage)++)return std::nullopt;return Message{win->procedure,hwnd,0x000f,0,0};},[](X86Guest& g){g.set_eax(1);});
        return 1;});
    U("SetFocus",4,1,[&W,arg](HleCall& c)->std::uint32_t{
        const auto hwnd=arg(c,0);const auto previous=W.focus;if(hwnd&&!W.find(hwnd))return 0;if(previous==hwnd)return previous;
        W.focus=hwnd;auto stage=std::make_shared<int>(0);
        send_chain(c.guest,4,[&W,hwnd,previous,stage](X86Guest&,std::optional<std::uint32_t>)->std::optional<Message>{
            for(;;)switch((*stage)++){case 0:if(!previous||!W.find(previous))continue;return Message{W.find(previous)->procedure,previous,0x0008,hwnd,0};
                case 1:if(!hwnd||!W.find(hwnd))return std::nullopt;return Message{W.find(hwnd)->procedure,hwnd,0x0007,previous,0};default:return std::nullopt;}},
            [previous](X86Guest& g){g.set_eax(previous);});
        return previous;});
    U("GetFocus",0,0,[&W](HleCall&)->std::uint32_t{return W.focus;});
    U("SetForegroundWindow",4,1,[&W,arg](HleCall& c)->std::uint32_t{
        auto* w=W.find(arg(c,0));if(!w)return 0;
        std::uint32_t top=arg(c,0);while(auto* parent=W.find(W.windows[top].parent))top=parent->handle; // a child brings its top-level window
        if(W.foreground==top)return 1;
        c.guest.stop("SetForegroundWindow activating another window is not modelled");return 0;});
    U("GetForegroundWindow",0,0,[&W](HleCall&)->std::uint32_t{return W.foreground;});
    U("GetActiveWindow",0,0,[&W](HleCall&)->std::uint32_t{return W.active;});
    U("SetWindowPos",28,7,[&W,arg](HleCall& c)->std::uint32_t{
        auto* w=W.find(arg(c,0));if(!w)return 0;const auto flags=arg(c,6);
        if(flags&~0x0217U){c.guest.stop("SetWindowPos flags "+std::to_string(flags)+" are not modelled");return 0;} // NOSIZE NOMOVE NOZORDER NOACTIVATE NOOWNERZORDER
        if(!(flags&2)){w->x=static_cast<std::int32_t>(arg(c,2));w->y=static_cast<std::int32_t>(arg(c,3));}
        if(!(flags&1)){w->width=static_cast<std::int32_t>(arg(c,4));w->height=static_cast<std::int32_t>(arg(c,5));}
        const auto hwnd=arg(c,0);const auto pos=c.guest.esp()-64; // a WINDOWPOS below the caller's stack, alive during the sends
        const std::uint32_t wp[7]={hwnd,arg(c,1),arg(c,2),arg(c,3),arg(c,4),arg(c,5),flags|0x14}; // (NOZORDER/NOACTIVATE as Windows adds for children)
        auto block=std::make_shared<std::uint32_t>(0);auto stage=std::make_shared<int>(0);
        send_chain(c.guest,28,[&W,hwnd,wp,stage,block](X86Guest& g,std::optional<std::uint32_t>)->std::optional<Message>{
            auto* win=W.find(hwnd);if(!win)return std::nullopt;
            if(!*block){*block=g.esp()-0x400;g.write(*block,wp,28);}
            switch((*stage)++){case 0:return Message{win->procedure,hwnd,0x0046,0,*block};case 1:return Message{win->procedure,hwnd,0x0047,0,*block};default:return std::nullopt;}},
            [](X86Guest& g){g.set_eax(1);});
        (void)pos;return 1;});
    const auto rect_out=[](HleCall& c,std::uint32_t at,std::int32_t l,std::int32_t t,std::int32_t r,std::int32_t b){const std::int32_t v[4]={l,t,r,b};c.guest.write(at,v,16);};
    U("GetClientRect",8,2,[&W,arg,rect_out](HleCall& c)->std::uint32_t{auto* w=W.find(arg(c,0));if(!w)return 0;const auto s=client_size(*w);rect_out(c,arg(c,1),0,0,s.w,s.h);return 1;});
    U("GetWindowRect",8,2,[&W,arg,rect_out](HleCall& c)->std::uint32_t{auto* w=W.find(arg(c,0));if(!w)return 0;
        std::int32_t x=w->x,y=w->y;for(auto* parent=W.find(w->parent);parent;parent=W.find(parent->parent)){const auto o=client_origin(*parent);x+=o.x;y+=o.y;}
        rect_out(c,arg(c,1),x,y,x+w->width,y+w->height);return 1;});
    const auto client_to_screen=[&W](std::uint32_t hwnd)->std::optional<Point>{
        auto* w=W.find(hwnd);if(!w)return std::nullopt;Point o=client_origin(*w);
        for(auto* parent=W.find(w->parent);parent;parent=W.find(parent->parent)){const auto q=client_origin(*parent);o.x+=q.x;o.y+=q.y;}return o;};
    U("ClientToScreen",8,2,[client_to_screen,arg](HleCall& c)->std::uint32_t{const auto o=client_to_screen(arg(c,0));if(!o)return 0;
        std::int32_t pt[2]={};c.guest.read(arg(c,1),pt,8);pt[0]+=o->x;pt[1]+=o->y;c.guest.write(arg(c,1),pt,8);return 1;});
    U("ScreenToClient",8,2,[client_to_screen,arg](HleCall& c)->std::uint32_t{const auto o=client_to_screen(arg(c,0));if(!o)return 0;
        std::int32_t pt[2]={};c.guest.read(arg(c,1),pt,8);pt[0]-=o->x;pt[1]-=o->y;c.guest.write(arg(c,1),pt,8);return 1;});
    U("IsWindowVisible",4,1,[&W,arg](HleCall& c)->std::uint32_t{auto* w=W.find(arg(c,0));return w&&w->visible?1:0;});
    U("IsIconic",4,1,[](HleCall&)->std::uint32_t{return 0;});
    U("IsWindow",4,1,[&W,arg](HleCall& c)->std::uint32_t{return W.find(arg(c,0))?1:0;});
    U("GetParent",4,1,[&W,arg](HleCall& c)->std::uint32_t{auto* w=W.find(arg(c,0));return w?w->parent:0;});
    U("GetDesktopWindow",0,0,[](HleCall&)->std::uint32_t{return 0x00010000U;});
    // painting: device contexts are tokens, nothing is drawn
    U("BeginPaint",8,2,[P,&W,arg](HleCall& c)->std::uint32_t{
        auto* w=W.find(arg(c,0));if(!w)return 0;w->needs_paint=false;const auto s=client_size(*w);
        std::uint32_t ps[16]={};ps[0]=P->new_handle();ps[1]=0;ps[2]=0;ps[3]=0;ps[4]=static_cast<std::uint32_t>(s.w);ps[5]=static_cast<std::uint32_t>(s.h);
        c.guest.write(arg(c,1),ps,64);return ps[0];});
    U("EndPaint",8,2,[](HleCall&)->std::uint32_t{return 1;});
    U("GetDC",4,1,[P](HleCall&)->std::uint32_t{return P->new_handle();});
    U("ReleaseDC",8,2,[](HleCall&)->std::uint32_t{return 1;});
    U("FillRect",12,3,[](HleCall&)->std::uint32_t{return 1;});
    U("ValidateRect",8,2,[&W,arg](HleCall& c)->std::uint32_t{if(auto* w=W.find(arg(c,0)))w->needs_paint=false;return 1;});
    U("InvalidateRect",12,3,[&W,arg](HleCall& c)->std::uint32_t{if(auto* w=W.find(arg(c,0)))w->needs_paint=true;return 1;});
    // the message queue: posted messages first, then WM_PAINT for a window that needs it, then WM_QUIT
    const auto take=[&W](std::uint32_t hwnd_filter,bool remove)->std::optional<HleMessage>{
        for(auto i=W.queue.begin();i!=W.queue.end();++i)if(!hwnd_filter||i->hwnd==hwnd_filter){auto m=*i;if(remove)W.queue.erase(i);return m;}
        for(auto& [h,w]:W.windows)if(!w.destroyed&&w.visible&&w.needs_paint&&(!hwnd_filter||h==hwnd_filter))return HleMessage{h,0x000f,0,0,0,0,0};
        if(W.quit_posted){if(remove)W.quit_posted=false;return HleMessage{0,0x0012,W.quit_code,0,0,0,0};}
        return std::nullopt;};
    const auto put_msg=[](HleCall& c,std::uint32_t at,const HleMessage& m){const std::uint32_t v[7]={m.hwnd,m.message,m.wparam,m.lparam,m.time,static_cast<std::uint32_t>(m.x),static_cast<std::uint32_t>(m.y)};c.guest.write(at,v,28);};
    const auto peek=[P,arg,take,put_msg](HleCall& c)->std::uint32_t{
        if(arg(c,2)||arg(c,3)){c.guest.stop("PeekMessage with a message filter range is not modelled");return 0;}
        const auto m=take(arg(c,1),(arg(c,4)&1)!=0);if(!m)return 0;auto msg=*m;msg.time=static_cast<std::uint32_t>(P->now_100ns/10000);put_msg(c,arg(c,0),msg);return 1;};
    U("PeekMessageA",20,5,peek);U("PeekMessageW",20,5,peek);
    const auto get=[take,put_msg,arg](HleCall& c)->std::uint32_t{
        const auto m=take(arg(c,1),true);if(!m){c.guest.stop("GetMessage with an empty queue would block: no host events are fed to the guest yet");return 0;}
        put_msg(c,arg(c,0),*m);return m->message==0x0012?0:1;};
    U("GetMessageA",16,4,get);U("GetMessageW",16,4,get);
    // TranslateMessage: WM_KEYDOWN of a key with a character posts WM_CHAR (same lParam) right behind it (US layout:
    // letters lower case unless Shift xor Caps Lock; digits; Enter, Escape, Space, Backspace, Tab). Nonzero for key messages.
    U("TranslateMessage",4,1,[&W,arg](HleCall& c)->std::uint32_t{
        std::uint32_t m[4]={};c.guest.read(arg(c,0),m,16);
        if(m[1]<0x0100||m[1]>0x0105)return 0;
        if(m[1]!=0x0100)return 1;
        const auto vk=m[2]&0xff;std::uint32_t ch=0;
        if(vk>='A'&&vk<='Z')ch=((W.keys[0x10]&0x80)!=0)!=((W.keys[0x14]&1)!=0)?vk:vk+0x20;
        else if(vk>='0'&&vk<='9'&&!(W.keys[0x10]&0x80))ch=vk;
        else if(vk==0x0d||vk==0x1b||vk==0x20||vk==0x08||vk==0x09)ch=vk;
        if(ch){HleMessage msg;msg.hwnd=m[0];msg.message=0x0102;msg.wparam=ch;msg.lparam=m[3];W.queue.insert(W.queue.begin(),msg);}
        return 1;});
    const auto dispatch=[&W,arg](HleCall& c)->std::uint32_t{
        std::uint32_t m[4]={};c.guest.read(arg(c,0),m,16);auto* w=W.find(m[0]);if(!w)return 0;
        c.guest.redirect(w->procedure,{m[0],m[1],m[2],m[3]},4,[](X86Guest&){});return 0;};
    U("DispatchMessageA",4,1,dispatch);U("DispatchMessageW",4,1,dispatch);
    const auto send=[&W,arg](HleCall& c)->std::uint32_t{auto* w=W.find(arg(c,0));if(!w)return 0;
        c.guest.redirect(w->procedure,{arg(c,0),arg(c,1),arg(c,2),arg(c,3)},16,[](X86Guest&){});return 0;};
    U("SendMessageA",16,4,send);U("SendMessageW",16,4,send);
    const auto post=[&W,arg](HleCall& c)->std::uint32_t{if(arg(c,0)&&!W.find(arg(c,0)))return 0;W.queue.push_back({arg(c,0),arg(c,1),arg(c,2),arg(c,3),0,0,0});return 1;};
    U("PostMessageA",16,4,post);U("PostMessageW",16,4,post);
    U("PostQuitMessage",4,1,[&W,arg](HleCall& c)->std::uint32_t{W.quit_posted=true;W.quit_code=arg(c,0);return 0;});
    U("TranslateAcceleratorA",12,3,[](HleCall&)->std::uint32_t{return 0;});U("TranslateAcceleratorW",12,3,[](HleCall&)->std::uint32_t{return 0;});
    const auto set_text=[P,&W,arg](bool wide){return [P,&W,arg,wide](HleCall& c)->std::uint32_t{auto* w=W.find(arg(c,0));if(!w)return 0;
        w->text=(wide?P->wide_at(arg(c,1)):P->string_at(arg(c,1))).value_or("");return 1;};};
    U("SetWindowTextA",8,2,set_text(false));U("SetWindowTextW",8,2,set_text(true));
    U("MessageBoxA",16,4,[P,arg](HleCall& c)->std::uint32_t{c.guest.stop("MessageBoxA(\""+P->string_at(arg(c,1)).value_or("?")+"\", \""+P->string_at(arg(c,2)).value_or("?")+"\")");return 0;});
    U("MessageBoxW",16,4,[P,arg](HleCall& c)->std::uint32_t{c.guest.stop("MessageBoxW(\""+P->wide_at(arg(c,1)).value_or("?")+"\", \""+P->wide_at(arg(c,2)).value_or("?")+"\")");return 0;});
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

namespace {
// Set-1 scan codes of the US layout by virtual key (0: no key).
std::uint32_t us_scan_code(std::uint32_t vk){
    static const std::map<std::uint32_t,std::uint32_t> table={
        {0x08,0x0e},{0x09,0x0f},{0x0d,0x1c},{0x10,0x2a},{0x11,0x1d},{0x12,0x38},{0x13,0x45},{0x14,0x3a},{0x1b,0x01},{0x20,0x39},
        {0x21,0x49},{0x22,0x51},{0x23,0x4f},{0x24,0x47},{0x25,0x4b},{0x26,0x48},{0x27,0x4d},{0x28,0x50},{0x2d,0x52},{0x2e,0x53},
        {0x30,0x0b},{0x31,0x02},{0x32,0x03},{0x33,0x04},{0x34,0x05},{0x35,0x06},{0x36,0x07},{0x37,0x08},{0x38,0x09},{0x39,0x0a},
        {0x41,0x1e},{0x42,0x30},{0x43,0x2e},{0x44,0x20},{0x45,0x12},{0x46,0x21},{0x47,0x22},{0x48,0x23},{0x49,0x17},{0x4a,0x24},
        {0x4b,0x25},{0x4c,0x26},{0x4d,0x32},{0x4e,0x31},{0x4f,0x18},{0x50,0x19},{0x51,0x10},{0x52,0x13},{0x53,0x1f},{0x54,0x14},
        {0x55,0x16},{0x56,0x2f},{0x57,0x11},{0x58,0x2d},{0x59,0x15},{0x5a,0x2c},
        {0x60,0x52},{0x61,0x4f},{0x62,0x50},{0x63,0x51},{0x64,0x4b},{0x65,0x4c},{0x66,0x4d},{0x67,0x47},{0x68,0x48},{0x69,0x49},
        {0x6a,0x37},{0x6b,0x4e},{0x6d,0x4a},{0x6e,0x53},{0x6f,0x35},
        {0x70,0x3b},{0x71,0x3c},{0x72,0x3d},{0x73,0x3e},{0x74,0x3f},{0x75,0x40},{0x76,0x41},{0x77,0x42},{0x78,0x43},{0x79,0x44},{0x7a,0x57},{0x7b,0x58},
        {0xa0,0x2a},{0xa1,0x36},{0xa2,0x1d},{0xa3,0x1d},{0xa4,0x38},{0xa5,0x38},
        {0xba,0x27},{0xbb,0x0d},{0xbc,0x33},{0xbd,0x0c},{0xbe,0x34},{0xbf,0x35},{0xc0,0x29},{0xdb,0x1a},{0xdc,0x2b},{0xdd,0x1b},{0xde,0x28}};
    const auto it=table.find(vk);return it==table.end()?0:it->second;}
bool extended_key(std::uint32_t vk){return (vk>=0x21&&vk<=0x28)||vk==0x2d||vk==0x2e||vk==0x6f||vk==0xa3||vk==0xa5;}
}
void HleProcess::host_key(std::uint32_t vk,bool down){
    vk&=0xff;auto& k=windows.keys[vk];const bool was_down=(k&0x80)!=0;
    if(down){if(!was_down)k^=1;k|=0x80;}else k&=0x7f;
    // lParam: repeat count 1, scan code, extended bit, previous state (bit 30), transition (bit 31).
    const auto lparam=1U|(us_scan_code(vk)<<16)|(extended_key(vk)?1U<<24:0U)|(was_down?1U<<30:0U)|(down?0U:1U<<31);
    const auto target=windows.focus?windows.focus:windows.active;
    if(!target)return; // no window has the keyboard: the key changes the state only
    HleMessage m;m.hwnd=target;m.message=down?0x0100U:0x0101U;m.wparam=vk;m.lparam=lparam;m.time=static_cast<std::uint32_t>(now_100ns/10000);
    windows.queue.push_back(m);
}
namespace {
void register_hle_user32_desktop(HleProcess& p){
    auto& r=p.hle();auto* P=&p;
    const auto arg=[](HleCall& c,unsigned i){return c.argument(i).value_or(0);};
    const auto U=[&](const char* name,std::uint32_t pops,unsigned logged,std::function<std::uint32_t(HleCall&)> body){r.add("USER32.dll",name,{pops,logged,std::move(body)});};
    const auto desktop=[P](){P->assume("the desktop is one 1920x1080 monitor at 96 DPI with Windows 10 metrics (caption 23, sizing frame 4 + padded border 4, fixed frame 3), work area 1920x1040");};
    U("GetSystemMetrics",4,1,[desktop](HleCall& c)->std::uint32_t{desktop();
        switch(c.argument(0).value_or(0xffff)){
        case 0:case 16:case 78:return screen_width;case 1:case 79:return screen_height;case 17:return 1017;case 76:case 77:return 0;
        case 2:case 3:return 17;case 4:return caption;case 5:case 6:return 1;case 7:case 8:return fixed_frame;case 11:case 12:case 13:case 14:return 32;
        case 15:return 20;case 19:return 1;case 23:return 0;case 32:case 33:return frame;case 34:return 136;case 35:return 39;case 36:case 37:return 4;
        case 49:case 50:return 16;case 68:case 69:return 4;case 80:return 1;case 92:return padded;case 0x1000:return 0;case 43:return 5;case 75:return 1;
        default:c.guest.stop("GetSystemMetrics("+std::to_string(c.argument(0).value_or(0))+") is not modelled");return 0;}});
    U("AdjustWindowRectEx",16,4,[desktop,arg](HleCall& c)->std::uint32_t{desktop();
        if(arg(c,2)){c.guest.stop("AdjustWindowRectEx with a menu is not modelled");return 0;}
        std::int32_t r4[4]={};c.guest.read(arg(c,0),r4,16);const auto i=insets(arg(c,1),arg(c,3));
        r4[0]-=i[0];r4[1]-=i[1];r4[2]+=i[2];r4[3]+=i[3];c.guest.write(arg(c,0),r4,16);return 1;});
    auto screensaver=std::make_shared<std::uint32_t>(1);
    const auto spi=[screensaver,arg](HleCall& c)->std::uint32_t{
        switch(arg(c,0)){
        case 0x10:c.guest.set_u32(arg(c,2),*screensaver);return 1;           // SPI_GETSCREENSAVEACTIVE
        case 0x11:*screensaver=arg(c,1)?1:0;return 1;                          // SPI_SETSCREENSAVEACTIVE
        case 0x30:{const std::int32_t r4[4]={0,0,screen_width,1040};c.guest.write(arg(c,2),r4,16);return 1;} // SPI_GETWORKAREA
        default:c.guest.stop("SystemParametersInfo action "+std::to_string(arg(c,0))+" is not modelled");return 0;}};
    U("SystemParametersInfoA",16,4,spi);U("SystemParametersInfoW",16,4,spi);
    U("AdjustWindowRect",12,3,[desktop,arg](HleCall& c)->std::uint32_t{desktop();
        if(arg(c,2)){c.guest.stop("AdjustWindowRect with a menu is not modelled");return 0;}
        std::int32_t r4[4]={};c.guest.read(arg(c,0),r4,16);const auto i=insets(arg(c,1),0);
        r4[0]-=i[0];r4[1]-=i[1];r4[2]+=i[2];r4[3]+=i[3];c.guest.write(arg(c,0),r4,16);return 1;});
    constexpr std::uint32_t monitor=0x00010001U;
    U("EnumDisplayMonitors",16,4,[P,desktop,arg](HleCall& c)->std::uint32_t{desktop();
        if(arg(c,0)||arg(c,1)){c.guest.stop("EnumDisplayMonitors with a DC or clip rectangle is not modelled");return 0;}
        const auto rect=P->memory().allocate(16).value_or(0);const std::int32_t r4[4]={0,0,screen_width,screen_height};c.guest.write(rect,r4,16);
        c.guest.redirect(arg(c,2),{monitor,0,rect,arg(c,3)},16,[P,rect](X86Guest& g){P->memory().free(rect);g.set_eax(1);});return 1;});
    const auto monitor_info=[desktop,arg](HleCall& c)->std::uint32_t{desktop();
        if(arg(c,0)!=monitor)return 0;std::uint32_t size=0;c.guest.read(arg(c,1),&size,4);
        const std::int32_t v[9]={static_cast<std::int32_t>(size),0,0,screen_width,screen_height,0,0,screen_width,1040};c.guest.write(arg(c,1),v,36);
        const std::uint32_t primary=1;c.guest.write(arg(c,1)+36,&primary,4);
        if(size>=104){const std::uint8_t name[64]={'\\',0,'\\',0,'.',0,'\\',0,'D',0,'I',0,'S',0,'P',0,'L',0,'A',0,'Y',0,'1',0};c.guest.write(arg(c,1)+40,name,64);}
        return 1;};
    U("GetMonitorInfoA",8,2,monitor_info);U("GetMonitorInfoW",8,2,monitor_info);
    U("MonitorFromWindow",8,2,[](HleCall&)->std::uint32_t{return monitor;});U("MonitorFromPoint",12,3,[](HleCall&)->std::uint32_t{return monitor;});
    U("MonitorFromRect",8,2,[](HleCall&)->std::uint32_t{return monitor;});
    // resources: icons, cursors, accelerators and stock objects are handle tokens (nothing is drawn)
    for(const char* name:{"LoadIconA","LoadIconW","LoadCursorA","LoadCursorW","LoadAcceleratorsA","LoadAcceleratorsW"})U(name,8,2,[P](HleCall&)->std::uint32_t{return P->new_handle();});
    U("LoadImageW",24,6,[P](HleCall&)->std::uint32_t{return P->new_handle();});U("LoadImageA",24,6,[P](HleCall&)->std::uint32_t{return P->new_handle();});
    r.add("GDI32.dll","GetStockObject",{4,1,[](HleCall& c)->std::uint32_t{return 0x01900000U+c.argument(0).value_or(0);}});
    // DWM: Windows 10 has no DWMWA_WINDOW_CORNER_PREFERENCE (33): E_INVALIDARG, as the loader's DisableRoundedCorners expects there.
    r.add("dwmapi.dll","DwmSetWindowAttribute",{16,4,[](HleCall& c)->std::uint32_t{return c.argument(1).value_or(0)>=33?0x80070057U:0;}});
    // cursor and keyboard state: nothing pressed, the cursor at the screen centre and shown
    auto cursor_count=std::make_shared<std::int32_t>(0);
    U("ShowCursor",4,1,[cursor_count,arg](HleCall& c)->std::uint32_t{*cursor_count+=arg(c,0)?1:-1;return static_cast<std::uint32_t>(*cursor_count);});
    U("SetCursor",4,1,[](HleCall&)->std::uint32_t{return 0;});U("ClipCursor",4,1,[](HleCall&)->std::uint32_t{return 1;});
    U("GetCursorPos",4,1,[arg](HleCall& c)->std::uint32_t{const std::int32_t pt[2]={screen_width/2,screen_height/2};c.guest.write(arg(c,0),pt,8);return 1;});
    U("SetCursorPos",8,2,[](HleCall&)->std::uint32_t{return 1;});
    // Key state from host input (HleProcess::host_key); nothing pressed until the host sends keys.
    U("GetAsyncKeyState",4,1,[P,arg](HleCall& c)->std::uint32_t{return P->windows.keys[arg(c,0)&0xff]&0x80?0xffff8000U:0;});
    U("GetKeyState",4,1,[P,arg](HleCall& c)->std::uint32_t{const auto k=P->windows.keys[arg(c,0)&0xff];return (k&0x80?0xffff8000U:0)|(k&1);});
    U("GetKeyboardState",4,1,[P,arg](HleCall& c)->std::uint32_t{c.guest.write(arg(c,0),P->windows.keys.data(),256);return 1;});
    // MapVirtualKey(code, type): 0 VK -> scan code, 1 scan code -> VK, 2 VK -> unshifted character, 3 scan code -> VK (left/right).
    const auto map_virtual_key=[P,arg](HleCall& c)->std::uint32_t{
        P->assume("keyboard layout: US (scan codes and characters of the US layout; the reference machine's layout was not captured)");
        const auto code=arg(c,0),type=arg(c,1);
        switch(type){
        case 0:return us_scan_code(code);
        case 1:case 3:for(std::uint32_t vk=1;vk<256;++vk)if(us_scan_code(vk)==code&&(type==3||(vk!=0xa0&&vk!=0xa1&&vk!=0xa2&&vk!=0xa3&&vk!=0xa4&&vk!=0xa5)))return vk;return 0;
        case 2:if((code>='0'&&code<='9')||(code>='A'&&code<='Z'))return code;if(code==0x20||code==0x0d||code==0x08||code==0x09||code==0x1b)return code;return 0;
        default:c.guest.stop("MapVirtualKey type "+std::to_string(type)+" is not modelled");return 0;}};
    U("MapVirtualKeyW",8,2,map_virtual_key);U("MapVirtualKeyA",8,2,map_virtual_key);
}
}
} // namespace sadx

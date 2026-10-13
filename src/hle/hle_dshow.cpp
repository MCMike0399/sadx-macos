// DirectShow for the guest, headless: the FilterGraph the movie player 513ed0 builds for system\*.MPG
// (CoCreateInstance of CLSID_FilterGraph for IGraphBuilder; AddFilter of the game's own TEXTURERENDERER;
// AddSourceFilter of the movie; FindPin "Output"; Render; IBasicAudio::put_Volume; IMediaControl Run/Stop;
// IMediaEvent GetEvent/FreeEventParams polled every frame by 513d50 until EC_COMPLETE).
// What is modelled: the graph's interfaces on one object (shared reference count), the file source filter and
// its output pin, the movie's duration from the file (src/loader/mpeg_timing), stream time on the virtual
// clock, EC_COMPLETE once that time reaches the duration, and the graph's references to the filters added to
// it (AddRef + JoinFilterGraph on AddFilter; JoinFilterGraph(NULL) + Release when the graph goes).
// What is not: decoding and pin connection. Render connects nothing, so the game's renderer never receives a
// media type or a sample: no video frame is drawn and no audio plays (reported). Every method the game has
// not been seen to call stops the guest by name.
#include "hle/hle_com.hpp"
#include "loader/mpeg_timing.hpp"
#include <deque>
#include <fstream>
#include <iterator>
#include <memory>
namespace sadx {
namespace {
constexpr const char* clsid_filter_graph="e436ebb3-524f-11ce-9f53-0020af0ba770";
constexpr std::uint32_t s_ok=0,e_abort=0x80004004U,vfw_e_not_found=0x80040216U,e_outofmemory=0x8007000eU;
constexpr std::uint32_t ec_complete=1;
// The graph's interfaces by IID. IMediaEvent and IMediaEventEx share the Ex object (its vtable extends IMediaEvent's).
const std::map<std::string,std::string>& graph_interfaces(){
    static const std::map<std::string,std::string> m={
        {"00000000-0000-0000-c000-000000000046","IGraphBuilder"},  // IUnknown
        {"56a8689f-0ad4-11ce-b03a-0020af0ba770","IGraphBuilder"},  // IFilterGraph (IGraphBuilder extends it)
        {"56a868a9-0ad4-11ce-b03a-0020af0ba770","IGraphBuilder"},
        {"56a868b1-0ad4-11ce-b03a-0020af0ba770","IMediaControl"},
        {"56a868b2-0ad4-11ce-b03a-0020af0ba770","IMediaPosition"},
        {"56a868b3-0ad4-11ce-b03a-0020af0ba770","IBasicAudio"},
        {"56a868b6-0ad4-11ce-b03a-0020af0ba770","IMediaEventEx"},  // IMediaEvent
        {"56a868c0-0ad4-11ce-b03a-0020af0ba770","IMediaEventEx"},
        {"56a868a2-0ad4-11ce-b03a-0020af0ba770","IMediaEventSink"}};
    return m;}
struct Graph {
    std::uint32_t references=0;
    std::map<std::string,std::uint32_t> objects;   // interface -> guest object
    std::vector<std::uint32_t> filters;            // added with AddFilter (the graph holds a reference)
    std::optional<std::uint64_t> duration_100ns;   // from AddSourceFilter
    enum class State {stopped,paused,running} state=State::stopped;
    std::uint64_t position_100ns=0,run_started_100ns=0;
    bool complete_sent=false;
    std::deque<std::array<std::uint32_t,3>> events;
    std::uint32_t volume=0,balance=0;
};
struct DShow {
    HleProcess& p;
    std::map<std::string,std::unique_ptr<HleComInterface>> interfaces;
    std::map<std::uint32_t,std::shared_ptr<Graph>> graph_of; // any graph interface object -> its graph
    explicit DShow(HleProcess& process):p(process){}
    std::uint64_t stream_time(const Graph& g)const{
        auto t=g.position_100ns;if(g.state==Graph::State::running)t+=p.now_100ns-g.run_started_100ns;
        return g.duration_100ns?std::min(t,*g.duration_100ns):t;}
    void deliver_due(Graph& g){
        if(g.state==Graph::State::running&&g.duration_100ns&&!g.complete_sent&&stream_time(g)>=*g.duration_100ns){
            g.events.push_back({ec_complete,s_ok,0});g.complete_sent=true;}} // param2 0: the graph's default handling sends one EC_COMPLETE
    std::optional<std::uint32_t> object(const std::shared_ptr<Graph>& g,const std::string& name){
        if(const auto it=g->objects.find(name);it!=g->objects.end())return it->second;
        const auto o=interfaces.at(name)->create(8);if(!o)return std::nullopt;
        g->objects[name]=*o;graph_of[*o]=g;return o;}
};
std::uint32_t vtable_slot(X86Guest& g,std::uint32_t object,std::uint32_t offset){return g.u32(g.u32(object).value_or(0)+offset).value_or(0);}
// Guest calls one after another from an HLE method (stdcall, `pops` = the method's argument bytes), then EAX = result.
struct GuestCall {std::uint32_t target;std::vector<std::uint32_t> arguments;std::string what;};
void call_chain(X86Guest& g,std::shared_ptr<std::vector<GuestCall>> calls,std::size_t i,std::uint32_t pops,std::uint32_t result){
    if(i==calls->size()){g.set_eax(result);return;}
    const auto& c=(*calls)[i];
    g.redirect(c.target,c.arguments,pops,[calls,i,result](X86Guest& g2){
        if((*calls)[i].what=="JoinFilterGraph"&&static_cast<std::int32_t>(g2.reg(19))<0){g2.stop("IBaseFilter::JoinFilterGraph of a filter added to the DirectShow graph failed");return;}
        call_chain(g2,calls,i+1,0,result);});
}
}
void register_hle_dshow(HleProcess& p){
    auto d=std::make_shared<DShow>(p);auto* P=&p;
    const auto graph=[d](HleCall& c)->std::shared_ptr<Graph>{const auto it=d->graph_of.find(c.argument(0).value_or(0));return it==d->graph_of.end()?nullptr:it->second;};
    const auto arg=[](HleCall& c,unsigned i){return c.argument(i).value_or(0);};
    const auto stop=[](std::string name){return [name](HleCall& c)->std::uint32_t{c.guest.stop(name+" is not modelled");return 0x80004005U;};};
    // IUnknown of every graph interface: one reference count for the object.
    const auto unknown=[d,graph,arg](const std::string& name)->std::vector<HleComMethod>{return {
        {"QueryInterface",12,[d,graph,arg,name](HleCall& c)->std::uint32_t{
            const auto iid=guid_text(c.guest,arg(c,1));const auto it=graph_interfaces().find(iid);auto g=graph(c);
            if(!g||it==graph_interfaces().end()){c.guest.stop(name+"::QueryInterface("+iid+") is not modelled");return 0x80004002U;}
            const auto o=d->object(g,it->second);if(!o)return e_outofmemory;
            ++g->references;c.guest.set_u32(arg(c,2),*o);return s_ok;}},
        {"AddRef",4,[graph](HleCall& c)->std::uint32_t{auto g=graph(c);return g?++g->references:0;}},
        {"Release",4,[d,graph](HleCall& c)->std::uint32_t{
            auto g=graph(c);if(!g||!g->references)return 0;
            if(--g->references)return g->references;
            // The last reference: the graph removes its filters (JoinFilterGraph(NULL)) and releases them.
            auto calls=std::make_shared<std::vector<GuestCall>>();
            for(const auto f:g->filters){calls->push_back({vtable_slot(c.guest,f,0x34),{f,0,0},"JoinFilterGraph"});calls->push_back({vtable_slot(c.guest,f,8),{f},"Release"});}
            for(const auto& [name,o]:g->objects)d->graph_of.erase(o);
            g->filters.clear();g->objects.clear();
            if(!calls->empty())call_chain(c.guest,calls,0,4,0);
            return 0;}}};};
    const auto dispatch=[stop](const std::string& name)->std::vector<HleComMethod>{return {
        {"GetTypeInfoCount",8,stop(name+"::GetTypeInfoCount")},{"GetTypeInfo",16,stop(name+"::GetTypeInfo")},
        {"GetIDsOfNames",24,stop(name+"::GetIDsOfNames")},{"Invoke",36,stop(name+"::Invoke")}};};
    const auto make=[d,P](const std::string& name,std::vector<HleComMethod> methods){d->interfaces[name]=std::make_unique<HleComInterface>(*P,name,std::move(methods));};
    const auto join=[](std::vector<HleComMethod> a,std::vector<HleComMethod> b){a.insert(a.end(),b.begin(),b.end());return a;};

    // The file source ("File Source (Async.)" with its "Output" pin): own reference counts, nothing else is used.
    make("IPin(DShowFileSource)",{com_query_interface_stop("IPin(DShowFileSource)"),com_add_ref(),com_release(),
        {"Connect",12,stop("IPin::Connect")},{"ReceiveConnection",12,stop("IPin::ReceiveConnection")},{"Disconnect",4,stop("IPin::Disconnect")},
        {"ConnectedTo",8,stop("IPin::ConnectedTo")},{"ConnectionMediaType",8,stop("IPin::ConnectionMediaType")},{"QueryPinInfo",8,stop("IPin::QueryPinInfo")},
        {"QueryDirection",8,stop("IPin::QueryDirection")},{"QueryId",8,stop("IPin::QueryId")},{"QueryAccept",8,stop("IPin::QueryAccept")},
        {"EnumMediaTypes",8,stop("IPin::EnumMediaTypes")},{"QueryInternalConnections",12,stop("IPin::QueryInternalConnections")},
        {"EndOfStream",4,stop("IPin::EndOfStream")},{"BeginFlush",4,stop("IPin::BeginFlush")},{"EndFlush",4,stop("IPin::EndFlush")},
        {"NewSegment",24,stop("IPin::NewSegment")}});
    make("IBaseFilter(DShowFileSource)",{com_query_interface_stop("IBaseFilter(DShowFileSource)"),com_add_ref(),com_release(),
        {"GetClassID",8,stop("IBaseFilter::GetClassID")},{"Stop",4,stop("IBaseFilter::Stop")},{"Pause",4,stop("IBaseFilter::Pause")},
        {"Run",12,stop("IBaseFilter::Run")},{"GetState",12,stop("IBaseFilter::GetState")},{"SetSyncSource",8,stop("IBaseFilter::SetSyncSource")},
        {"GetSyncSource",8,stop("IBaseFilter::GetSyncSource")},{"EnumPins",8,stop("IBaseFilter::EnumPins")},
        {"FindPin",12,[d,P,arg](HleCall& c)->std::uint32_t{
            const auto id=P->wide_at(arg(c,1));
            if(!id||*id!="Output"){c.guest.set_u32(arg(c,2),0);return vfw_e_not_found;}
            const auto pin=d->interfaces.at("IPin(DShowFileSource)")->create(8);if(!pin)return e_outofmemory;
            c.guest.set_u32(arg(c,2),*pin);return s_ok;}},
        {"QueryFilterInfo",8,stop("IBaseFilter::QueryFilterInfo")},{"JoinFilterGraph",12,stop("IBaseFilter::JoinFilterGraph")},
        {"QueryVendorInfo",8,stop("IBaseFilter::QueryVendorInfo")}});

    make("IGraphBuilder",join(unknown("IGraphBuilder"),{
        {"AddFilter",12,[d,graph,arg](HleCall& c)->std::uint32_t{
            auto g=graph(c);const auto f=arg(c,1);if(!g||!f){c.guest.stop("IFilterGraph::AddFilter without a graph or a filter");return 0x80004003U;}
            g->filters.push_back(f);
            auto calls=std::make_shared<std::vector<GuestCall>>(std::vector<GuestCall>{
                {vtable_slot(c.guest,f,4),{f},"AddRef"},{vtable_slot(c.guest,f,0x34),{f,g->objects.at("IGraphBuilder"),arg(c,2)},"JoinFilterGraph"}});
            call_chain(c.guest,calls,0,12,s_ok);return s_ok;}},
        {"RemoveFilter",8,stop("IFilterGraph::RemoveFilter")},{"EnumFilters",8,stop("IFilterGraph::EnumFilters")},
        {"FindFilterByName",12,stop("IFilterGraph::FindFilterByName")},{"ConnectDirect",16,stop("IFilterGraph::ConnectDirect")},
        {"Reconnect",8,stop("IFilterGraph::Reconnect")},{"Disconnect",8,stop("IFilterGraph::Disconnect")},
        {"SetDefaultSyncSource",4,stop("IFilterGraph::SetDefaultSyncSource")},{"Connect",12,stop("IGraphBuilder::Connect")},
        {"Render",8,[P](HleCall&)->std::uint32_t{
            P->assume("DirectShow (headless): IGraphBuilder::Render connects no decoder and no renderer, so the game's TEXTURERENDERER filter receives no media type and no sample: no movie frame is drawn and no movie audio plays");
            return s_ok;}},
        {"RenderFile",12,stop("IGraphBuilder::RenderFile")},
        {"AddSourceFilter",16,[d,P,graph,arg](HleCall& c)->std::uint32_t{
            auto g=graph(c);const auto name=P->wide_at(arg(c,1));
            if(!g||!name){c.guest.stop("IGraphBuilder::AddSourceFilter: unreadable file name");return 0x80004003U;}
            const auto host=P->host_path(*name);
            if(!host||!std::filesystem::is_regular_file(*host)){P->trace_file("AddSourceFilter",*name,"not found");c.guest.set_u32(arg(c,3),0);return 0x80070002U;}
            std::ifstream f(*host,std::ios::binary);std::string error;
            const auto timing=mpeg1_timing({std::istreambuf_iterator<char>(f),{}},&error);
            if(!timing){c.guest.stop("IGraphBuilder::AddSourceFilter("+*name+"): "+error);return 0x80004005U;}
            P->trace_file("AddSourceFilter",*name,"MPEG-1 "+std::to_string(timing->duration_100ns/10000)+" ms");
            P->assume("DirectShow (headless): a movie's duration is its MPEG-1 video PTS span plus one frame period (the MPEG-1 splitter's own rule was not captured)");
            g->duration_100ns=timing->duration_100ns;g->complete_sent=false;g->position_100ns=0;
            const auto source=d->interfaces.at("IBaseFilter(DShowFileSource)")->create(8);if(!source)return e_outofmemory;
            c.guest.set_u32(arg(c,3),*source);return s_ok;}},
        {"SetLogFile",8,stop("IGraphBuilder::SetLogFile")},{"Abort",4,stop("IGraphBuilder::Abort")},
        {"ShouldOperationContinue",4,stop("IGraphBuilder::ShouldOperationContinue")}}));
    make("IMediaControl",join(join(unknown("IMediaControl"),dispatch("IMediaControl")),{
        {"Run",4,[d,graph,P](HleCall& c)->std::uint32_t{
            auto g=graph(c);if(!g)return 0x80004005U;
            P->assume("DirectShow (headless): stream time runs on the virtual clock from IMediaControl::Run; EC_COMPLETE is queued when it reaches the movie's duration");
            if(g->state!=Graph::State::running){g->state=Graph::State::running;g->run_started_100ns=P->now_100ns;}return s_ok;}},
        {"Pause",4,[d,graph](HleCall& c)->std::uint32_t{auto g=graph(c);if(!g)return 0x80004005U;g->position_100ns=d->stream_time(*g);g->state=Graph::State::paused;return s_ok;}},
        {"Stop",4,[d,graph](HleCall& c)->std::uint32_t{auto g=graph(c);if(!g)return 0x80004005U;g->position_100ns=d->stream_time(*g);g->state=Graph::State::stopped;return s_ok;}},
        {"StopWhenReady",4,stop("IMediaControl::StopWhenReady")},
        {"GetState",12,[graph,arg](HleCall& c)->std::uint32_t{auto g=graph(c);if(!g)return 0x80004005U;c.guest.set_u32(arg(c,2),static_cast<std::uint32_t>(g->state));return s_ok;}},
        {"RenderFile",8,stop("IMediaControl::RenderFile")},{"AddSourceFilter",12,stop("IMediaControl::AddSourceFilter")},
        {"get_FilterCollection",8,stop("IMediaControl::get_FilterCollection")},{"get_RegFilterCollection",8,stop("IMediaControl::get_RegFilterCollection")}}));
    make("IMediaEventEx",join(join(unknown("IMediaEventEx"),dispatch("IMediaEventEx")),{
        {"GetEventHandle",8,stop("IMediaEvent::GetEventHandle")},
        {"GetEvent",20,[d,graph,arg](HleCall& c)->std::uint32_t{
            auto g=graph(c);if(!g)return 0x80004005U;d->deliver_due(*g);
            if(g->events.empty()){if(arg(c,4)){c.guest.stop("IMediaEvent::GetEvent with a timeout and no event is not modelled");return e_abort;}return e_abort;}
            const auto e=g->events.front();g->events.pop_front();
            c.guest.set_u32(arg(c,1),e[0]);c.guest.set_u32(arg(c,2),e[1]);c.guest.set_u32(arg(c,3),e[2]);return s_ok;}},
        {"WaitForCompletion",12,stop("IMediaEvent::WaitForCompletion")},{"CancelDefaultHandling",8,stop("IMediaEvent::CancelDefaultHandling")},
        {"RestoreDefaultHandling",8,stop("IMediaEvent::RestoreDefaultHandling")},
        {"FreeEventParams",16,[](HleCall&)->std::uint32_t{return s_ok;}}, // EC_COMPLETE's parameters own nothing
        {"SetNotifyWindow",16,stop("IMediaEventEx::SetNotifyWindow")},{"SetNotifyFlags",8,stop("IMediaEventEx::SetNotifyFlags")},
        {"GetNotifyFlags",8,stop("IMediaEventEx::GetNotifyFlags")}}));
    make("IMediaPosition",join(join(unknown("IMediaPosition"),dispatch("IMediaPosition")),{
        {"get_Duration",8,[graph,arg](HleCall& c)->std::uint32_t{auto g=graph(c);if(!g||!g->duration_100ns)return 0x80004005U;
            const double s=static_cast<double>(*g->duration_100ns)/1e7;c.guest.write(arg(c,1),&s,8);return s_ok;}},
        {"put_CurrentPosition",12,stop("IMediaPosition::put_CurrentPosition")},
        {"get_CurrentPosition",8,[d,graph,arg](HleCall& c)->std::uint32_t{auto g=graph(c);if(!g)return 0x80004005U;
            const double s=static_cast<double>(d->stream_time(*g))/1e7;c.guest.write(arg(c,1),&s,8);return s_ok;}},
        {"get_StopTime",8,stop("IMediaPosition::get_StopTime")},{"put_StopTime",12,stop("IMediaPosition::put_StopTime")},
        {"get_PrerollTime",8,stop("IMediaPosition::get_PrerollTime")},{"put_PrerollTime",12,stop("IMediaPosition::put_PrerollTime")},
        {"put_Rate",12,stop("IMediaPosition::put_Rate")},{"get_Rate",8,stop("IMediaPosition::get_Rate")},
        {"CanSeekForward",8,stop("IMediaPosition::CanSeekForward")},{"CanSeekBackward",8,stop("IMediaPosition::CanSeekBackward")}}));
    make("IBasicAudio",join(join(unknown("IBasicAudio"),dispatch("IBasicAudio")),{
        {"put_Volume",8,[graph,arg](HleCall& c)->std::uint32_t{auto g=graph(c);if(!g)return 0x80004005U;g->volume=arg(c,1);return s_ok;}},
        {"get_Volume",8,[graph,arg](HleCall& c)->std::uint32_t{auto g=graph(c);if(!g)return 0x80004005U;c.guest.set_u32(arg(c,1),g->volume);return s_ok;}},
        {"put_Balance",8,[graph,arg](HleCall& c)->std::uint32_t{auto g=graph(c);if(!g)return 0x80004005U;g->balance=arg(c,1);return s_ok;}},
        {"get_Balance",8,[graph,arg](HleCall& c)->std::uint32_t{auto g=graph(c);if(!g)return 0x80004005U;c.guest.set_u32(arg(c,1),g->balance);return s_ok;}}}));
    make("IMediaEventSink",join(unknown("IMediaEventSink"),{
        {"Notify",16,[graph,arg](HleCall& c)->std::uint32_t{auto g=graph(c);if(!g)return 0x80004005U;g->events.push_back({arg(c,1),arg(c,2),arg(c,3)});return s_ok;}}}));

    // CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC, riid, ppv).
    const auto previous=*p.hle().find("ole32.dll","CoCreateInstance");
    p.hle().add("ole32.dll","CoCreateInstance",{20,5,[d,previous,arg](HleCall& c)->std::uint32_t{
        if(guid_text(c.guest,arg(c,0))!=clsid_filter_graph)return previous.body(c);
        if(arg(c,1)){c.guest.stop("CoCreateInstance(CLSID_FilterGraph) aggregated is not modelled");return 0x80040110U;}
        const auto iid=guid_text(c.guest,arg(c,3));const auto it=graph_interfaces().find(iid);
        if(it==graph_interfaces().end()){c.guest.stop("CoCreateInstance(CLSID_FilterGraph) for "+iid+" is not modelled");return 0x80004002U;}
        auto g=std::make_shared<Graph>();
        if(!d->object(g,"IGraphBuilder"))return e_outofmemory;
        const auto o=d->object(g,it->second);if(!o)return e_outofmemory;
        g->references=1;c.guest.set_u32(arg(c,4),*o);return s_ok;}});
}
} // namespace sadx

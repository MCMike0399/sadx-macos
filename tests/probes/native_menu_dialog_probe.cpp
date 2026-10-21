#include "game/menu_dialog.hpp"
#include <algorithm>
namespace dialog=sadx::menu_dialog;
namespace {
struct Stop{};
struct Calls final:dialog::Calls {
    const std::uint32_t *input;std::uint32_t *out;sadx::TaskHandle &canonical,other;
    Calls(const std::uint32_t *i,std::uint32_t *o,sadx::TaskHandle &task,sadx::TaskHandle replacement):input(i),out(o),canonical(task),other(std::move(replacement)){}
    void play_sound(std::int32_t cue)override{out[80]=1;out[81]=static_cast<std::uint32_t>(cue);if(input[45])throw Stop{};if(input[44])canonical=other;}
};
}
extern "C" void sadx_native_menu_dialog_probe(const std::uint32_t *i,std::uint32_t *o){
    std::fill(o,o+256,0);sadx::TaskScheduler scheduler;auto original=scheduler.append_task(3,nullptr,{}).task;auto other=scheduler.append_task(3,nullptr,{}).task;
    auto work=std::make_shared<dialog::Work>();auto replacement=std::make_shared<dialog::Work>();replacement->phase=static_cast<std::int32_t>(i[46]);replacement->cursor=static_cast<std::uint8_t>(i[47]);other->auxiliary_native_work=replacement;
    dialog::Catalog catalog(3);
    for(unsigned n=0;n<3;++n){catalog[n].choice_count=static_cast<std::int8_t>(i[5]);if(i[6])catalog[n].sounds=std::make_shared<sadx::title_menu::DialogSounds>(sadx::title_menu::DialogSounds{static_cast<std::int32_t>(i[7]+n),0,0,0,0});}
    work->parameters=&catalog[i[8]%3];work->phase=static_cast<std::int32_t>(i[1]);work->cursor=static_cast<std::uint8_t>(i[2]);work->previous_choice=static_cast<std::int32_t>(i[3]);work->repeat_frames=static_cast<std::int16_t>(i[4]);work->opening_frames=static_cast<std::int16_t>(i[49]);
    for(unsigned n=0;n<16;++n)work->choice_ids[n]=static_cast<std::int8_t>(i[28+n]);
    if(i[58])original->auxiliary_native_work=work;auto canonical=i[57]?original:sadx::TaskHandle{};Calls calls(i,o,canonical,other);dialog::Owner owner{canonical};std::array<std::int8_t,16> bytes{};for(unsigned n=0;n<16;++n)bytes[n]=static_cast<std::int8_t>(i[12+n]);dialog::Choices choices{};
    if(i[11]==1)choices={bytes.data(),bytes.size()};else if(i[11]==2)choices={work->choice_ids.data(),work->choice_ids.size()};else if(i[11]==3)choices={work->choice_ids.data()+1,work->choice_ids.size()-1};
    try{switch(i[0]){case 0:dialog::open_with_cursor(catalog[0],static_cast<std::int8_t>(i[10]),choices,owner,calls);break;case 1:dialog::open(catalog[0],owner,calls);break;case 2:dialog::request(static_cast<std::int32_t>(i[9]),catalog,owner,calls);break;case 3:dialog::set_cursor(static_cast<std::int8_t>(i[10]),owner);break;case 4:o[1]=static_cast<std::uint32_t>(dialog::result(owner));break;}}
    catch(const Stop&){o[2]=1;}
    o[0]=work->parameters==&catalog[0]?1:work->parameters==&catalog[1]?2:3;o[3]=static_cast<std::uint32_t>(work->phase);o[4]=work->cursor;o[5]=static_cast<std::uint32_t>(work->previous_choice);o[6]=static_cast<std::uint16_t>(work->repeat_frames);o[7]=static_cast<std::uint16_t>(work->opening_frames);for(unsigned n=0;n<16;++n)o[8+n]=static_cast<std::uint8_t>(work->choice_ids[n]);o[24]=static_cast<std::uint32_t>(dialog::result(owner));o[25]=!canonical?0:canonical==original?1:2;
}

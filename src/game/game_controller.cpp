#include "game/game_controller.hpp"
namespace sadx {
std::optional<std::uint8_t> GameLogicController::read_byte(std::size_t at)const noexcept {return at<byte_count?bytes_[at]:std::optional<std::uint8_t>{};}
std::optional<std::uint16_t> GameLogicController::read_u16(std::size_t at)const noexcept {
    if(at>byte_count-2||!bytes_[at]||!bytes_[at+1])return {};return std::uint16_t(*bytes_[at])|(std::uint16_t(*bytes_[at+1])<<8);
}
std::optional<std::uint32_t> GameLogicController::read_u32(std::size_t at)const noexcept {
    if(at>byte_count-4)return {};std::uint32_t value=0;for(std::size_t i=0;i<4;++i){if(!bytes_[at+i])return {};value|=std::uint32_t(*bytes_[at+i])<<(i*8);}return value;
}
bool GameLogicController::write_byte(std::size_t at,std::optional<std::uint8_t> v)noexcept {if(at>=byte_count)return false;bytes_[at]=v;return true;}
bool GameLogicController::write_u16(std::size_t at,std::uint16_t value)noexcept {if(at>byte_count-2)return false;for(std::size_t i=0;i<2;++i)bytes_[at+i]=static_cast<std::uint8_t>(value>>(i*8));return true;}
bool GameLogicController::write_u32(std::size_t at,std::uint32_t value)noexcept {if(at>byte_count-4)return false;for(std::size_t i=0;i<4;++i)bytes_[at+i]=static_cast<std::uint8_t>(value>>(i*8));return true;}
bool GameLogicController::forget(std::size_t at,std::size_t count)noexcept {if(at>byte_count||count>byte_count-at)return false;for(std::size_t i=0;i<count;++i)bytes_[at+i].reset();return true;}
bool init_controllers_40f3a0(const GameLogicController& source,const std::array<std::shared_ptr<GameLogicController>,4>& raw,GameLogicControllers& pointers,GameControllerInitGlobals& g){
    for(auto& w:g.words_at_3b0e768)w=0;
    for(const auto& record:raw)if(!record)return false;
    for(std::size_t b=0;b<GameLogicController::byte_count;++b)raw[0]->write_byte(b,source.read_byte(b));
    g.words_at_3b0e344[0]=0;
    for(std::size_t r=1;r<raw.size();++r)for(std::size_t b=0;b<GameLogicController::byte_count;++b)raw[r]->write_byte(b,raw[r-1]->read_byte(b));
    g.words_at_3b0e344[1]=0;g.words_at_3b0e344[2]=0;g.words_at_3b0e344[3]=0;
    for(std::size_t i=0;i<pointers.size();++i)pointers[i]=raw[i];
    for(std::size_t i=0;i<4;++i){g.words_at_3b0e9b8[i]=0;g.bytes_at_3b0e9a4[i]=0;}
    for(auto& w:g.words_at_3b0e990)w=0;
    return true;
}
} // namespace sadx

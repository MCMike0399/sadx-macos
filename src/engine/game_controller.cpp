#include "engine/game_controller.hpp"
namespace sadx {
GameLogicController GameLogicController::from_input_values(const ControllerInputValues& values) {
    GameLogicController record;
    for (std::size_t index = 0; index < byte_count; ++index)
        record.write_byte(index, 0);
    record.write_u32(0, values.id);
    record.write_u32(4, values.support);
    record.write_u32(8, values.held);
    record.write_u32(12, values.not_held);
    record.write_u32(16, values.pressed);
    record.write_u32(20, values.released);
    record.write_u16(24, values.right_trigger);
    record.write_u16(26, values.left_trigger);
    record.write_u16(28, static_cast<std::uint16_t>(values.left_x));
    record.write_u16(30, static_cast<std::uint16_t>(values.left_y));
    record.write_u16(32, static_cast<std::uint16_t>(values.right_x));
    record.write_u16(34, static_cast<std::uint16_t>(values.right_y));
    record.write_u32(44, values.previous_held);
    return record;
}
std::optional<ControllerInputValues> GameLogicController::input_values() const noexcept {
    // The public byte adapter may represent unknown reference data. Native callers
    // initialize every byte through from_input_values before requesting a snapshot.
    for (const auto& byte : bytes_)
        if (!byte)
            return {};
    const auto word = [&](std::size_t field) {
        return *read_u32(field);
    };
    const auto pressure = [&](std::size_t field) {
        return *read_u16(field);
    };
    const auto axis = [&](std::size_t field) {
        const auto value = pressure(field);
        return static_cast<std::int16_t>(value < 32768 ? static_cast<int>(value)
                                                     : static_cast<int>(value) - 65536);
    };
    return ControllerInputValues{word(0), word(4), word(8), word(12), word(16), word(20),
                                 pressure(24), pressure(26), axis(28), axis(30), axis(32),
                                 axis(34), word(44)};
}

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

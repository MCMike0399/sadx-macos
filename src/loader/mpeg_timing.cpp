#include "loader/mpeg_timing.hpp"
namespace sadx {
namespace {
std::uint64_t time_stamp(const std::uint8_t* b){ // 4 marker bits, 33 bits with three marker bits between
    return (std::uint64_t((b[0]>>1)&7)<<30)|(std::uint64_t(b[1])<<22)|(std::uint64_t(b[2]>>1)<<15)|(std::uint64_t(b[3])<<7)|(b[4]>>1);}
bool fail(std::string* error,const std::string& why){if(error)*error=why;return false;}
}
std::optional<MpegTiming> mpeg1_timing(const std::vector<std::uint8_t>& s,std::string* error){
    // frame_rate_code 1..8 (ISO/IEC 11172-2 table 2-D.4): 23.976, 24, 25, 29.97, 30, 50, 59.94, 60.
    static const std::uint32_t rates[9][2]={{0,1},{24000,1001},{24,1},{25,1},{30000,1001},{30,1},{50,1},{60000,1001},{60,1}};
    MpegTiming t;bool have_rate=false;std::size_t at=0;
    const auto start=[&](std::size_t i){return i+4<=s.size()&&s[i]==0&&s[i+1]==0&&s[i+2]==1;};
    while(at+4<=s.size()){
        if(!start(at)){if(error)*error="no start code at byte "+std::to_string(at);return std::nullopt;}
        const auto code=s[at+3];
        if(code==0xb9)break; // ISO 11172 end code
        if(code==0xba){
            if(at+12>s.size()||(s[at+4]&0xf0)!=0x20){if(error)*error="not an MPEG-1 pack header at byte "+std::to_string(at);return std::nullopt;}
            at+=12;continue;}
        if(code<0xbb){if(error)*error="unexpected start code at byte "+std::to_string(at);return std::nullopt;}
        if(at+6>s.size()){if(error)*error="truncated packet at byte "+std::to_string(at);return std::nullopt;}
        const std::size_t length=(std::size_t(s[at+4])<<8)|s[at+5];const auto end=at+6+length;
        if(end>s.size()){if(error)*error="packet past the end at byte "+std::to_string(at);return std::nullopt;}
        if(code==0xe0){
            auto i=at+6;
            while(i<end&&s[i]==0xff)++i;            // stuffing
            if(i<end&&(s[i]&0xc0)==0x40)i+=2;       // STD buffer
            std::size_t payload=i+1;
            if(i+5<=end&&(s[i]&0xe0)==0x20){        // '0010' PTS or '0011' PTS DTS
                const auto pts=time_stamp(&s[i]);
                if(!t.video_packets_with_pts++)t.first_pts=t.last_pts=pts;
                else{t.first_pts=std::min(t.first_pts,pts);t.last_pts=std::max(t.last_pts,pts);}
                payload=i+((s[i]&0xf0)==0x30?10:5);}
            if(!have_rate)for(auto k=payload;k+8<=end;++k)if(s[k]==0&&s[k+1]==0&&s[k+2]==1&&s[k+3]==0xb3){
                const auto code_rate=s[k+7]&15;if(code_rate<1||code_rate>8)break;
                t.width=(std::uint32_t(s[k+4])<<4)|(s[k+5]>>4);t.height=(std::uint32_t(s[k+5]&15)<<8)|s[k+6];
                t.frame_rate_numerator=rates[code_rate][0];t.frame_rate_denominator=rates[code_rate][1];have_rate=true;break;}
        }
        at=end;
    }
    if(!t.video_packets_with_pts){fail(error,"no video packet carries a PTS");return std::nullopt;}
    if(!have_rate){fail(error,"no video sequence header in the packet payloads");return std::nullopt;}
    // 90 kHz ticks to 100 ns: x 1000 / 9; the frame period in 100 ns: 10^7 x den / num (both truncated).
    t.duration_100ns=(t.last_pts-t.first_pts)*1000/9+std::uint64_t(10000000)*t.frame_rate_denominator/t.frame_rate_numerator;
    return t;
}
} // namespace sadx

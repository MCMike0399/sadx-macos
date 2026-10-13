#include "cpu/crt_argument.hpp"
#include <cstring>
namespace sadx {
X87Value load_crt_binary64_argument(double input,X87Context& fp){
    std::uint64_t u;std::memcpy(&u,&input,8);
    if((u&0x7ff0000000000000ULL)!=0x7ff0000000000000ULL)return fp.from_double(input);
    const auto sign=static_cast<std::uint16_t>((u>>48)&0x8000U);
    return fp.from_canonical_ext80(static_cast<std::uint16_t>(sign|0x7fffU),
        0x8000000000000000ULL|((u&0xfffffffffffffULL)<<11));
}
} // namespace sadx

#include "cpu/x87_math.hpp"

#include <array>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <thread>
#include <vector>

namespace {

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

std::uint32_t bits(float value) {
    std::uint32_t output;
    std::memcpy(&output, &value, sizeof(output));
    return output;
}

float from_bits(std::uint32_t input) {
    float output;
    std::memcpy(&output, &input, sizeof(output));
    return output;
}

float cancellation(sadx::X87Context& fp) {
    const auto z = fp.from_float(0x1p60F);
    const auto y = fp.from_float(1.0F);
    const auto x = fp.from_float(-0x1p60F);
    return fp.to_float(fp.add(fp.add(z, y), x));
}

void test_precision_and_range() {
    sadx::X87Context fp53(0x027f), fp64(0x037f), fp24(0x007f);
    require(cancellation(fp53) == 0, "53-bit cancellation must lose the unit");
    require(cancellation(fp64) == 1, "64-bit cancellation must preserve the unit");
    require(cancellation(fp24) == 0, "24-bit cancellation must lose the unit");
    for (auto* fp : {&fp24, &fp53, &fp64}) {
        const auto one = fp->from_float(1);
        const auto small = fp->from_float(0x1p-25F);
        const auto difference = fp->subtract(fp->add(one, small), one);
        require(fp->to_float(difference) == (fp == &fp24 ? 0 : 0x1p-25F),
                "precision control must affect each arithmetic operation");
        fp->clear_exceptions();
        const auto maximum = fp->from_float(std::numeric_limits<float>::max());
        const auto square = fp->multiply(maximum, maximum);
        require((square.sign_exponent() & 0x7fffU) != 0x7fffU,
                "reduced precision must preserve extended exponent range");
        require((fp->exception_flags() & 0x08U) == 0, "no ext80 overflow for squared binary32 max");
        require(std::isinf(fp->to_float(square)), "binary32 store must overflow separately");
        require((fp->exception_flags() & 0x28U) == 0x28U, "store overflow and precision flags");
    }
    bool rejected = false;
    try { sadx::X87Context reserved(0x017f); }
    catch (const sadx::X87Error& error) {
        rejected = error.failure() == sadx::X87Failure::reserved_precision_control;
    }
    require(rejected, "reserved precision encoding must not silently become 64-bit");
}

void test_rounding() {
    // At 24-bit significand, 1 + 2^-24 is exactly halfway between floats.
    const std::array<std::uint32_t, 4> positive{0x3f800000, 0x3f800000, 0x3f800001, 0x3f800000};
    const std::array<std::uint32_t, 4> negative{0xbf800000, 0xbf800001, 0xbf800000, 0xbf800000};
    for (std::uint16_t rounding = 0; rounding < 4; ++rounding) {
        sadx::X87Context fp(static_cast<std::uint16_t>(0x007f | (rounding << 10U)));
        require(bits(fp.to_float(fp.add(fp.from_float(1), fp.from_float(0x1p-24F)))) == positive[rounding],
                "positive arithmetic rounding mode");
        require(bits(fp.to_float(fp.add(fp.from_float(-1), fp.from_float(-0x1p-24F)))) == negative[rounding],
                "negative arithmetic rounding mode");
    }
    sadx::X87Context fp(0x037f);
    const auto precise_half = fp.add(fp.from_float(1), fp.from_float(0x1p-24F));
    require(bits(fp.to_float(precise_half)) == 0x3f800000, "nearest binary32 store");
    fp.set_control_word(0x0b7f); // RC up, PC extended
    require(fp.control_word() == 0x0b7f && bits(fp.to_float(precise_half)) == 0x3f800001,
            "captured control word must govern stores and remain intact");
}

void test_nonfinite_and_exceptions() {
    sadx::X87Context fp(0x037f);
    for (const auto word : {0U, 0x80000000U, 0x3f800000U, 0x7f800000U, 0xff800000U,
                           0x7fc12345U, 0xffc54321U, 1U, 0x007fffffU}) {
        require(bits(fp.to_float(fp.from_float(from_bits(word)))) == word,
                "binary32 round-trip must preserve finite/zero/infinity/quiet-NaN payload bits");
    }
    require((fp.exception_flags() & 0x02U) != 0, "binary32 denormal load records denormal operand");
    fp.clear_exceptions();
    const auto signaling = fp.from_float(from_bits(0x7f812345));
    require((fp.exception_flags() & 0x01U) != 0, "signaling NaN conversion flags invalid");
    require(bits(fp.to_float(signaling)) == 0x7fc12345, "8086 signaling NaN payload is quieted, retained");
    require(bits(fp.to_float(fp.sqrt(fp.from_float(-0.0F)))) == 0x80000000,
            "sqrt negative zero preserves sign");
    require(fp.to_float(fp.sqrt(fp.from_float(9))) == 3, "exact square root");
    fp.clear_exceptions();
    require(std::isnan(fp.to_float(fp.sqrt(fp.from_float(-1)))), "masked negative root produces NaN");
    require((fp.exception_flags() & 0x01U) != 0, "masked invalid flag retained");
    fp.clear_exceptions();
    const auto tiny = fp.multiply(fp.from_float(std::numeric_limits<float>::denorm_min()),
                                  fp.from_float(0.5F));
    require(bits(fp.to_float(tiny)) == 0 && (fp.exception_flags() & 0x30U) == 0x30U,
            "tiny binary32 store records underflow and inexact rather than flushing input");

    sadx::X87Context invalid_unmasked(0x037e);
    bool stopped = false;
    try { (void)invalid_unmasked.sqrt(invalid_unmasked.from_float(-1)); }
    catch (const sadx::X87Error& error) {
        stopped = error.failure() == sadx::X87Failure::unmasked_exception && error.exceptions() == 1;
    }
    require(stopped && invalid_unmasked.exception_flags() == 1, "unmasked invalid must stop and retain flags");
    stopped = false;
    try { (void)invalid_unmasked.from_float(1); }
    catch (const sadx::X87Error& error) { stopped = error.failure() == sadx::X87Failure::unmasked_exception; }
    require(stopped, "pending unmasked exception must not silently disappear");
    invalid_unmasked.clear_exceptions();
    require(invalid_unmasked.to_float(invalid_unmasked.from_float(1)) == 1, "explicit flag clearing resumes evaluation");

    sadx::X87Context denormal_unmasked(0x037d);
    stopped = false;
    try { (void)denormal_unmasked.from_float(std::numeric_limits<float>::denorm_min()); }
    catch (const sadx::X87Error& error) { stopped = error.exceptions() == 2; }
    require(stopped, "unmasked denormal input must be explicit");
}

void test_context_scopes_and_threads() {
    sadx::X87Context fp53(0x027f), fp64(0x037f);
    bool missing = false;
    try { (void)sadx::X87Context::current(); }
    catch (const sadx::X87Error& error) { missing = error.failure() == sadx::X87Failure::missing_thread_context; }
    require(missing, "missing active context must not invent a game control word");
    {
        sadx::X87Scope outer(fp53);
        require(&sadx::X87Context::current() == &fp53, "outer active context");
        {
            sadx::X87Scope inner(fp64);
            require(cancellation(sadx::X87Context::current()) == 1, "nested selected precision");
        }
        require(&sadx::X87Context::current() == &fp53 && cancellation(fp53) == 0,
                "nested scope restores owner and precision");
        require(cancellation(fp64) == 1 && cancellation(fp53) == 0,
                "direct operations isolate same-thread context environments");
    }
    std::array<std::exception_ptr, 2> failures{};
    std::array<std::thread, 2> workers;
    for (std::size_t i = 0; i < workers.size(); ++i) {
        workers[i] = std::thread([&, i] {
            try {
                sadx::X87Context local(i == 0 ? 0x027f : 0x037f);
                sadx::X87Scope scope(local);
                for (int count = 0; count < 10000; ++count) {
                    require(cancellation(sadx::X87Context::current()) == (i == 0 ? 0 : 1),
                            "concurrent thread control-word contamination");
                    if ((count & 31) == 0) std::this_thread::yield();
                }
                bool foreign = false;
                try { (void)fp53.from_float(1); }
                catch (const sadx::X87Error& error) { foreign = error.failure() == sadx::X87Failure::cross_thread_context; }
                require(foreign, "foreign-thread context mutation must be rejected");
            } catch (...) { failures[i] = std::current_exception(); }
        });
    }
    for (auto& worker : workers) worker.join();
    for (const auto& failure : failures) if (failure) std::rethrow_exception(failure);
}

void test_signaling_comparison() {
    sadx::X87Context fp(0x037f);
    const auto zero = fp.from_float(0);
    require(fp.compare(zero, fp.from_float(-0.0F)) == sadx::X87Comparison::equal,
            "FCOM signed zeros equal");
    require(fp.compare(fp.from_float(-1), zero) == sadx::X87Comparison::less,
            "FCOM negative less");
    require(fp.compare(fp.from_float(std::numeric_limits<float>::denorm_min()), zero) ==
            sadx::X87Comparison::greater, "FCOM denormal input remains nonzero");
    require(fp.compare(fp.from_float(std::numeric_limits<float>::infinity()),
                       fp.from_float(-std::numeric_limits<float>::infinity())) ==
            sadx::X87Comparison::greater, "FCOM opposing infinities");
    for (const auto payload : {0x7fc12345U, 0x7f812345U, 0xffc54321U}) {
        const auto nan = fp.from_float(from_bits(payload));
        fp.clear_exceptions(); // remove any signaling-NaN load exception
        require(fp.compare(nan, zero) == sadx::X87Comparison::unordered &&
                (fp.exception_flags() & 1U) != 0, "FCOM all NaNs raise masked invalid");
        fp.clear_exceptions();
        require(fp.compare(zero, nan) == sadx::X87Comparison::unordered &&
                (fp.exception_flags() & 1U) != 0, "FCOM NaN second operand also invalid");
    }
    require(sadx::x87_comparison_status(sadx::X87Comparison::less) == 0x0100 &&
            sadx::x87_comparison_status(sadx::X87Comparison::equal) == 0x4000 &&
            sadx::x87_comparison_status(sadx::X87Comparison::greater) == 0 &&
            sadx::x87_comparison_status(sadx::X87Comparison::unordered) == 0x4500,
            "FCOM C0/C2/C3 encoding");
    sadx::X87Context unmasked(0x037e);
    bool stopped = false;
    try { (void)unmasked.compare(unmasked.from_float(from_bits(0x7fc12345)), unmasked.from_float(1)); }
    catch (const sadx::X87Error& error) {
        stopped = error.failure() == sadx::X87Failure::unmasked_exception && error.exceptions() == 1;
    }
    require(stopped && unmasked.exception_flags() == 1, "FCOM quiet NaN unmasked invalid stops");
    require(fp.control_word() == 0x037f, "comparison context does not inherit another control word");
    unmasked.clear_exceptions();
    stopped = false;
    try { (void)unmasked.compare(unmasked.from_float(from_bits(0x7f812345)), unmasked.from_float(1)); }
    catch (const sadx::X87Error& error) {
        stopped = error.failure() == sadx::X87Failure::unmasked_exception && error.exceptions() == 1;
    }
    require(stopped, "signaling-NaN load/FCOM pipeline must retain unmasked invalid");

    std::array<std::exception_ptr, 2> failures{};
    std::array<std::thread, 2> workers;
    for (std::size_t i = 0; i < workers.size(); ++i) {
        workers[i] = std::thread([&,i] {
            try {
                sadx::X87Context local(0x037f);
                for (int iteration = 0; iteration < 1000; ++iteration) {
                    local.clear_exceptions();
                    const auto first = local.from_float(i == 0 ? from_bits(0x7fc12345) : 1);
                    const auto compared = local.compare(first, local.from_float(0));
                    require(compared == (i == 0 ? sadx::X87Comparison::unordered : sadx::X87Comparison::greater),
                            "concurrent FCOM result contaminated");
                    require(local.exception_flags() == (i == 0 ? 1 : 0), "concurrent FCOM flags contaminated");
                    if ((iteration & 31) == 0) std::this_thread::yield();
                }
            } catch (...) { failures[i] = std::current_exception(); }
        });
    }
    for (auto& worker : workers) worker.join();
    for (const auto& failure : failures) if (failure) std::rethrow_exception(failure);
}

void test_division_and_integer_load_store() {
    for (const std::uint16_t pc : {0x007f,0x027f,0x037f}) {
        sadx::X87Context fp(pc);
        require(fp.to_float(fp.divide(fp.from_float(3),fp.from_float(2)))==1.5F,"exact ext80 division");
        fp.clear_exceptions();
        require(std::isinf(fp.to_float(fp.divide(fp.from_float(1),fp.from_float(0)))) &&
                (fp.exception_flags()&4U)!=0,"division records source zero denominator");
        const auto maximum=fp.from_int64(std::numeric_limits<std::int64_t>::max());
        require(fp.to_int64_current_rounding(maximum)==std::numeric_limits<std::int64_t>::max(),
                "FILD64 exact load ignores precision control");
        const auto minimum=fp.from_int64(std::numeric_limits<std::int64_t>::min());
        require(fp.to_int64_current_rounding(minimum)==std::numeric_limits<std::int64_t>::min(),"FILD64 negative bound");
        const auto loaded=fp.from_double(1.0+0x1p-40);
        require(fp.to_float(fp.subtract(loaded,fp.from_double(1)))==0x1p-40F,"FLD64 exact value preserved under all PCs");
    }
    const std::array<std::int64_t,4> positive{2,1,2,1},negative{-2,-2,-1,-1};
    for (std::uint16_t rc=0;rc<4;++rc) {
        sadx::X87Context fp(static_cast<std::uint16_t>(0x037f|(rc<<10U)));
        require(fp.to_int64_current_rounding(fp.from_double(1.5))==positive[rc] &&
                fp.to_int64_current_rounding(fp.from_double(-1.5))==negative[rc],"FISTP64 obeys current RC");
        require((fp.exception_flags()&0x20U)!=0,"FISTP64 inexact flag");
        fp.clear_exceptions();
        require(fp.to_int64_current_rounding(fp.from_float(std::numeric_limits<float>::infinity()))==
                std::numeric_limits<std::int64_t>::min() && (fp.exception_flags()&1U)!=0,
                "8086 masked invalid integer indefinite");
    }
}

void test_source_constant_import_and_double_store() {
    sadx::X87Context fp(0x037f);
    const auto half_pi=fp.from_canonical_ext80(0x3fff,0xc90fdaa22168c235ULL);
    require(half_pi.sign_exponent()==0x3fff && half_pi.significand()==0xc90fdaa22168c235ULL,
            "PI/2 source constant bits preserved");
    const double projected=fp.to_double(half_pi);
    std::uint64_t projected_bits;std::memcpy(&projected_bits,&projected,8);
    require(projected_bits==0x3ff921fb54442d18ULL,"source PI/2 binary64 store");
    const auto negative_nan=fp.from_canonical_ext80(0xffff,0xc000000000000000ULL);
    require(negative_nan.sign_exponent()==0xffff && negative_nan.significand()==0xc000000000000000ULL,
            "negative source qNaN bits preserved");
    fp.clear_exceptions();
    const auto quiet=fp.from_canonical_ext80(0x7fff,0xc00123456789abcdULL);
    const auto signaling=fp.from_canonical_ext80(0x7fff,0x800123456789abcdULL);
    require(quiet.significand()==0xc00123456789abcdULL && signaling.significand()==0x800123456789abcdULL &&
            fp.exception_flags()==0,"source import does not invent FLD80 quieting/flags");
    require(fp.compare(signaling,fp.from_float(0))==sadx::X87Comparison::unordered &&
            (fp.exception_flags()&1U)!=0,"canonical signaling ext80 FCOM raises invalid");
    for (const auto malformed : {std::pair<std::uint16_t,std::uint64_t>{0,0x8000000000000000ULL},
             {0x3fff,0x4000000000000000ULL},{0x7fff,0x4000000000000000ULL}}) {
        bool rejected=false;
        try { (void)fp.from_canonical_ext80(malformed.first,malformed.second); }
        catch(const sadx::X87Error& error) { rejected=error.failure()==sadx::X87Failure::noncanonical_ext80_source; }
        require(rejected,"unsupported source ext80 must not normalize silently");
    }
    fp.clear_exceptions();
    const auto tiny=fp.from_canonical_ext80(0,1);
    require(fp.compare(tiny,fp.from_float(0))==sadx::X87Comparison::greater &&
            (fp.exception_flags()&2U)!=0,"canonical ext80 subnormal operand retained");
    sadx::X87Context unmasked(0x037e);bool stopped=false;
    try { (void)unmasked.compare(unmasked.from_canonical_ext80(0x7fff,0x8000000000000001ULL),unmasked.from_float(0)); }
    catch(const sadx::X87Error& error) { stopped=error.failure()==sadx::X87Failure::unmasked_exception && error.exceptions()==1; }
    require(stopped,"actual signaling ext80 compare follows unmasked invalid");
}

void test_memory_operands_and_nan_priority() {
    const float signaling=from_bits(0x7f812345);
    for(std::uint16_t pc:{0x0000,0x0200,0x0300}) for(std::uint16_t rc:{0,0x400,0x800,0xc00}) {
        sadx::X87Context fp(static_cast<std::uint16_t>(pc|rc|0x7f));
        const auto indefinite=fp.from_canonical_ext80(0xffff,0xc000000000000000ULL);
        const auto memory_result=fp.add_float_memory(indefinite,signaling);
        require(bits(fp.to_float(memory_result))==0xffc00000 && (fp.exception_flags()&1U)!=0,
                "Intel x87 QNaN + signaling memory source selects QNaN, invalid retained");
        fp.clear_exceptions();
        const auto separately_loaded=fp.from_float(signaling);
        require(bits(fp.to_float(fp.add(indefinite,separately_loaded)))==0x7fc12345,
                "separate FLD32 remains different, selecting larger quiet payload");
        for(const auto result:{fp.subtract_float_memory(indefinite,signaling),
                 fp.reverse_subtract_float_memory(indefinite,signaling),
                 fp.multiply_float_memory(indefinite,signaling),
                 fp.divide_float_memory(indefinite,signaling),
                 fp.reverse_divide_float_memory(indefinite,signaling)}) {
            require(bits(fp.to_float(result))==0xffc00000,"all memory arithmetic preserves signaling priority");
        }
        fp.clear_exceptions();
        require(bits(fp.to_float(fp.add_float_memory(indefinite,std::numeric_limits<float>::denorm_min())))==0xffc00000 &&
                fp.exception_flags()==0,"quiet NaN has precedence over denormal memory operand");
        const auto tiny=fp.from_canonical_ext80(0,1);fp.clear_exceptions();
        require(std::isnan(fp.to_float(fp.add_float_memory(tiny,from_bits(0x7fc12345)))) && fp.exception_flags()==0,
                "quiet memory NaN precedes register-denormal handling");
        fp.clear_exceptions();
        require(fp.compare_float_memory(indefinite,std::numeric_limits<float>::denorm_min())==sadx::X87Comparison::unordered &&
                fp.exception_flags()==1,"FCOM invalid NaN precedes denormal");
        const auto one=fp.from_float(1);fp.clear_exceptions();
        require(bits(fp.to_float(fp.multiply_float_memory(one,std::numeric_limits<float>::denorm_min())))==1 &&
                fp.exception_flags()==2,"finite memory denormal arithmetic preserves value and DE");
        fp.clear_exceptions();
        require(std::isinf(fp.to_float(fp.reverse_divide_float_memory(fp.from_float(0),
                    std::numeric_limits<float>::denorm_min()))) && fp.exception_flags()==4,
                "division by zero precedes denormal source handling");
    }
    sadx::X87Context fp(0x037f);
    const std::uint64_t double_signal_bits=0x7ff0000000000123ULL;
    double double_signal;std::memcpy(&double_signal,&double_signal_bits,8);
    const auto indefinite=fp.from_canonical_ext80(0xffff,0xc000000000000000ULL);
    require(bits(fp.to_float(fp.add_double_memory(indefinite,double_signal)))==0xffc00000,
            "m64 signaling NaN classification is retained too");
    require(fp.to_float(fp.reverse_subtract_float_memory(fp.from_float(2),5))==3 &&
            fp.to_float(fp.reverse_divide_float_memory(fp.from_float(2),6))==3,
            "reverse operations mean memory minus/divided by register");
    sadx::X87Context unmasked(0x037e);bool stopped=false;
    try { (void)unmasked.add_float_memory(unmasked.from_canonical_ext80(0xffff,0xc000000000000000ULL),signaling); }
    catch(const sadx::X87Error& error) { stopped=error.exceptions()==1 && error.failure()==sadx::X87Failure::unmasked_exception; }
    require(stopped,"combined memory SNaN unmasked invalid does not return a fabricated result");
    sadx::X87Context denormal_unmasked(0x037d);
    const auto quiet=denormal_unmasked.from_canonical_ext80(0xffff,0xc000000000000000ULL);
    require(bits(denormal_unmasked.to_float(denormal_unmasked.add_float_memory(quiet,
                std::numeric_limits<float>::denorm_min())))==0xffc00000 &&
            denormal_unmasked.exception_flags()==0,
            "QNaN suppresses even an unmasked memory-denormal exception");
    require(std::isinf(denormal_unmasked.to_float(denormal_unmasked.reverse_divide_float_memory(
                denormal_unmasked.from_float(0),std::numeric_limits<float>::denorm_min()))) &&
            denormal_unmasked.exception_flags()==4,
            "masked zero-divide precedes unmasked memory-denormal exception");
    sadx::X87Context both_unmasked(0x037c);stopped=false;
    try { (void)both_unmasked.compare_float_memory(
            both_unmasked.from_canonical_ext80(0xffff,0xc000000000000000ULL),
            std::numeric_limits<float>::denorm_min()); }
    catch(const sadx::X87Error& error) { stopped=error.exceptions()==1; }
    require(stopped && both_unmasked.exception_flags()==1,
            "unmasked FCOM NaN invalid wins before memory-denormal detection");
}

void test_quiet_register_comparison() {
    sadx::X87Context fp(0x037f);
    const auto qnan=fp.from_canonical_ext80(0x7fff,0xc123450000000000ULL);
    const auto snan=fp.from_canonical_ext80(0x7fff,0x8123450000000000ULL);
    const auto zero=fp.from_float(0);
    require(fp.compare_quiet(qnan,zero)==sadx::X87Comparison::unordered&&fp.exception_flags()==0,
            "FUCOM quiet NaN is unordered without invalid");
    require(fp.compare_quiet(zero,qnan)==sadx::X87Comparison::unordered&&fp.exception_flags()==0,
            "FUCOM quiet second operand is also non-signaling");
    require(fp.compare_quiet(snan,qnan)==sadx::X87Comparison::unordered&&fp.exception_flags()==1,
            "FUCOM signaling source raises invalid even with quiet opponent");
    fp.clear_exceptions();
    require(fp.compare_quiet(qnan,fp.from_canonical_ext80(0,1))==sadx::X87Comparison::unordered&&
            fp.exception_flags()==0,"quiet NaN takes priority over register denormal in FUCOM");
    require(fp.compare_quiet(zero,zero.negated())==sadx::X87Comparison::equal,
            "FUCOM signed zeros compare equal");
    require(fp.compare_quiet(fp.from_float(-1),zero)==sadx::X87Comparison::less&&
            fp.compare_quiet(fp.from_float(1),zero)==sadx::X87Comparison::greater,
            "FUCOM preserves finite ordering");
    sadx::X87Context unmasked(0x037e);
    require(unmasked.compare_quiet(qnan,zero)==sadx::X87Comparison::unordered&&unmasked.exception_flags()==0,
            "quiet NaN does not manufacture an unmasked FUCOM trap");
    bool stopped=false;try{(void)unmasked.compare_quiet(snan,zero);}
    catch(const sadx::X87Error& e){stopped=e.failure()==sadx::X87Failure::unmasked_exception&&e.exceptions()==1;}
    require(stopped,"unmasked signaling FUCOM remains explicit");
}

void test_table_interpolation(const char* path) {
    std::ifstream input(path, std::ios::binary);
    std::array<float, 1025> table;
    input.read(reinterpret_cast<char*>(table.data()), sizeof(table));
    require(input.gcount() == static_cast<std::streamsize>(sizeof(table)) &&
            input.peek() == std::char_traits<char>::eof(), "requires exact original 4100-byte sine table");
    std::uint64_t digest = 0xcbf29ce484222325ULL;
    for (std::size_t i = 0; i < sizeof(table); ++i) {
        digest = (digest ^ reinterpret_cast<const unsigned char*>(table.data())[i]) * 0x100000001b3ULL;
    }
    require(digest == 0xb047ecf880b993c2ULL, "original table identity differs");
    for (const std::uint16_t word : {0x027f, 0x037f}) {
        sadx::X87Context fp(word);
        for (std::size_t i = 0; i < 1024; ++i) {
            for (unsigned fraction = 0; fraction < 16; ++fraction) {
                for (int sign : {-1, 1}) {
                    const float first = sign == 1 ? table[i] : -table[i];
                    const float second = sign == 1 ? table[i + 1] : -table[i + 1];
                    const auto difference = fp.subtract(fp.from_float(second), fp.from_float(first));
                    const auto weight = fp.multiply(fp.from_float(static_cast<float>(fraction)), fp.from_float(0.0625F));
                    const auto result = fp.add(fp.multiply(difference, weight), fp.from_float(first));
                    const float expected = static_cast<float>((static_cast<double>(second) - first) *
                        (static_cast<double>(fraction) * 0.0625) + first);
                    require(bits(fp.to_float(result)) == bits(expected),
                            "53/64-bit table interpolation must retain observed float projection");
                }
            }
        }
    }
}

} // namespace

int main(int argc, char** argv) {
    try {
        require(argc == 2, "usage: x87_math_test ORIGINAL_QUARTER_SINE_TABLE");
        test_precision_and_range();
        test_rounding();
        test_nonfinite_and_exceptions();
        test_context_scopes_and_threads();
        test_signaling_comparison();
        test_division_and_integer_load_store();
        test_source_constant_import_and_double_store();
        test_memory_operands_and_nan_priority();
        test_quiet_register_comparison();
        test_table_interpolation(argv[1]);
        std::cout << "Native software ext80: cancellation, 24/53/64-bit precision, rounding, "
                     "flags, payloads, ownership, threads and original-table interpolation passed.\n"
                     "Actual game control state and hardware exception delivery remain unverified.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

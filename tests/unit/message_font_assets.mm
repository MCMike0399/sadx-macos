#include "apps/native_message_font_assets.hpp"
#include <cassert>
#include <functional>
#include <fstream>
#include <iostream>
#include <unistd.h>

namespace app = sadx::native::app;
namespace text = sadx::message_text;
static std::size_t checks = 0;
static void write_metadata(const std::filesystem::path &root, NSDictionary *metadata) {
    NSData *data = [NSJSONSerialization dataWithJSONObject:metadata options:0 error:nil];
    assert(data);
    assert([data writeToFile:[NSString stringWithUTF8String:(root/"message-fonts.json").c_str()]
                 atomically:YES]);
}
// Synthetic records exercise the application format without any game assets.
static void make_fixture(const std::filesystem::path &root) {
    std::filesystem::create_directories(root/"message-fonts");
    NSMutableDictionary *fonts = [NSMutableDictionary dictionary];
    for (const auto &family : {std::string("wide"), std::string("single")}) {
        const std::size_t count = family == "wide" ? 8836 : 256;
        const auto relative = "message-fonts/" + family + "-glyphs.bin";
        std::vector<std::uint8_t> bytes(count * 72);
        std::ofstream file(root/relative, std::ios::binary);
        file.write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
        std::array<unsigned char, CC_SHA256_DIGEST_LENGTH> digest{};
        CC_SHA256(bytes.data(), static_cast<CC_LONG>(bytes.size()), digest.data());
        std::ostringstream hash, fallback;
        for (auto byte : digest) hash << std::hex << std::setw(2) << std::setfill('0') << unsigned(byte);
        for (unsigned byte = 0; byte < 72; ++byte)
            fallback << std::hex << std::setw(2) << std::setfill('0') << byte;
        fonts[[NSString stringWithUTF8String:family.c_str()]] = @{
            @"glyph_count": @(count), @"glyph_file": [NSString stringWithUTF8String:relative.c_str()],
            @"glyph_sha256": [NSString stringWithUTF8String:hash.str().c_str()],
            @"fallback_bytes": [NSString stringWithUTF8String:fallback.str().c_str()]};
    }
    NSMutableArray *languages = [NSMutableArray array], *widths = [NSMutableArray array];
    for (std::size_t index = 0; index < 5; ++index)
        [languages addObject:@{@"lead_first": @(index < 2 ? 128 : 0),
                              @"lead_last": @(index < 2 ? 255 : 0), @"remapping": NSNull.null}];
    for (unsigned index = 0; index < 224; ++index) [widths addObject:@12];
    write_metadata(root, @{@"version": @1, @"fonts": fonts, @"languages": languages,
                           @"widths": widths, @"saving_text": @"Now Saving..."});
}
struct Calls : text::LoadCalls {
    const sadx::native::MessageFontAssets &assets;
    text::State &state;
    std::vector<text::FontFamily> order;
    Calls(const sadx::native::MessageFontAssets &a, text::State &s) : assets(a), state(s) {}
    std::shared_ptr<const text::FontData> load(text::FontFamily family) override {
        order.push_back(family);
        assert(!state.directory.single && !state.directory.wide);
        if (family == text::FontFamily::Single) assert(state.loaded_wide == assets.wide);
        return family == text::FontFamily::Wide ? assets.wide : assets.single;
    }
};
int main(int argc, char **argv) {
    @autoreleasepool {
        assert(argc == 1 || argc == 2);
        char pattern[] = "/private/tmp/sadx-message-loader-XXXXXX";
        const auto root = std::filesystem::path(mkdtemp(pattern));
        const auto source = argc == 2 ? std::filesystem::path(argv[1]) : root/"source";
        if (argc == 1) make_fixture(source);
        std::filesystem::copy(source/"message-fonts", root/"message-fonts",
                              std::filesystem::copy_options::recursive);
        NSData *original = [NSData dataWithContentsOfFile:
            [NSString stringWithUTF8String:(source/"message-fonts.json").c_str()]];
        const auto fresh = [&]() -> NSMutableDictionary * {
            return [NSJSONSerialization JSONObjectWithData:original
                       options:NSJSONReadingMutableContainers error:nil];
        };
        const auto reject = [&](const char *name, const std::function<void(NSMutableDictionary *)> &mutate) {
            NSMutableDictionary *metadata = fresh(); mutate(metadata); write_metadata(root, metadata);
            bool failed = false;
            try { (void)app::load_message_font_assets(root); }
            catch (const std::exception &) { failed = true; }
            if (!failed) { std::cerr << "accepted malformed " << name << '\n'; std::abort(); }
            ++checks;
        };
        reject("missing fonts", [](auto m) { [m removeObjectForKey:@"fonts"]; });
        reject("wrong count", [](auto m) { m[@"fonts"][@"single"][@"glyph_count"] = @255; });
        reject("wide count", [](auto m) { m[@"fonts"][@"wide"][@"glyph_count"] = @8835; });
        reject("boolean version", [](auto m) { m[@"version"] = @YES; });
        reject("bad hash", [](auto m) { m[@"fonts"][@"single"][@"glyph_sha256"] = @"00"; });
        reject("absolute path", [&](auto m) {
            m[@"fonts"][@"single"][@"glyph_file"] = [NSString stringWithUTF8String:
                (source/"message-fonts/single-glyphs.bin").c_str()];
        });
        std::filesystem::copy_file(root/"message-fonts/single-glyphs.bin", root.parent_path()/(root.filename().string()+"-outside.bin"));
        const auto outside = root.parent_path()/(root.filename().string()+"-outside.bin");
        reject("parent escape", [&](auto m) { m[@"fonts"][@"single"][@"glyph_file"] =
            [NSString stringWithUTF8String:("../"+outside.filename().string()).c_str()]; });
        std::filesystem::create_symlink(outside, root/"escape.bin");
        reject("symlink escape", [](auto m) { m[@"fonts"][@"single"][@"glyph_file"] = @"escape.bin"; });
        reject("nul path", [](auto m) {
            const char path[] = "message-fonts/single-glyphs.bin\0hidden";
            m[@"fonts"][@"single"][@"glyph_file"] = [[NSString alloc] initWithBytes:path
                length:sizeof(path)-1 encoding:NSUTF8StringEncoding];
        });
        reject("short fallback", [](auto m) { m[@"fonts"][@"single"][@"fallback_bytes"] = @"00"; });
        reject("nonhex fallback", [](auto m) { m[@"fonts"][@"wide"][@"fallback_bytes"] =
            [@"z" stringByPaddingToLength:144 withString:@"0" startingAtIndex:0]; });
        reject("missing remap", [](auto m) { [m[@"languages"][0] removeObjectForKey:@"remapping"]; });
        reject("unimported remap", [](auto m) { m[@"languages"][0][@"remapping"] = @[]; });
        reject("wrong lead first", [](auto m) { m[@"languages"][0][@"lead_first"] = @127; });
        reject("wrong lead last", [](auto m) { m[@"languages"][2][@"lead_last"] = @255; });
        reject("width count", [](auto m) { [m[@"widths"] removeLastObject]; });
        reject("width zero", [](auto m) { m[@"widths"][0] = @0; });
        reject("width 25", [](auto m) { m[@"widths"][0] = @25; });
        reject("boolean width", [](auto m) { m[@"widths"][0] = @YES; });
        reject("saving nul", [](auto m) { const char value[] = "Now Saving...\0hidden";
            m[@"saving_text"] = [[NSString alloc] initWithBytes:value length:sizeof(value)-1
                encoding:NSUTF8StringEncoding]; });
        reject("saving wrong source string", [](auto m) { m[@"saving_text"] = @"Saving"; });
        write_metadata(root, fresh());
        const auto single = root/"message-fonts/single-glyphs.bin";
        const auto payload = [NSData dataWithContentsOfFile:[NSString stringWithUTF8String:single.c_str()]];
        std::ofstream(single, std::ios::binary).put('x');
        bool failed = false;
        try { (void)app::load_message_font_assets(root); } catch (const std::exception &) { failed = true; }
        assert(failed); ++checks;
        assert([payload writeToFile:[NSString stringWithUTF8String:single.c_str()] atomically:YES]);
        auto assets = app::load_message_font_assets(root);
        assert(assets.single->glyphs.size() == 256 && assets.wide->glyphs.size() == 8836);
        text::State state;
        state.directory.languages = assets.languages;
        std::int32_t language = 1;
        Calls calls(assets, state);
        text::initialize(state, language, calls);
        assert((calls.order == std::vector<text::FontFamily>{text::FontFamily::Wide, text::FontFamily::Single}));
        assert(state.widths == assets.expected_widths);
        assert(state.directory.single == assets.single && state.directory.wide == assets.wide);
        assert(assets.saving_text == "Now Saving...");
        const auto glyph = text::glyph('N', state, language).bitmap;
        const auto fallback = assets.single->fallback.bitmap;
        assets = {};
        std::filesystem::remove_all(root); std::filesystem::remove(outside);
        assert(text::glyph('N', state, language).bitmap == glyph);
        state.directory.wide.reset();
        assert(text::glyph(256, state, language).bitmap == fallback);
        std::cout << checks << " malformed asset cases rejected; actual glyphs/source initialization/order/widths/fallback/lifetimes pass\n";
    }
}

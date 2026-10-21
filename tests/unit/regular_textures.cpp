#include "game/regular_textures.hpp"
#include <cassert>
#include <memory>
#include <string>
#include <vector>

int main() {
    using namespace sadx::regular_textures;
    sadx::texture_lifetime::Memory first{}, second{};
    const auto original = std::make_shared<const std::string>("b32ascii");
    const auto replacement = std::make_shared<const std::string>("b32ascii_j");
    List list{{&first, &first, &first}, false, {original, original, original}, {7,8,9}};
    List *common = nullptr;
    std::array<List *,32> regular{};
    std::uint8_t resync = 0;
    std::int32_t language = 0;
    Localizations rules{{{0, "b32ascii", replacement}}};
    State state{resync, language, common, regular, rules};
    std::vector<std::string> calls;
    Services services{
        [&](std::string_view name) {
            assert(resync == 1);
            calls.emplace_back(name);
            return ArchiveResult{1, {&first, &second}};
        }, [&](List &target) {
            assert(resync == 1 && target.filenames[0] == replacement);
            calls.emplace_back("individual");
            resync = 0;
            return -1;
        }, [&](std::string_view text) { calls.emplace_back(text); }};
    state.scratch_attributes[0] = 11;
    state.scratch_attributes[1] = 12;
    load_archive("OBJ_REGULAR", &list, state, services);
    assert((list.entries == std::vector{&first,&second,&second}));
    assert((list.attributes == std::vector<std::uint32_t>{11,12,12}));
    assert(list.filenames[2] == original);
    load_no_name(list, state, services);
    assert(list.filenames[0] == original && list.filenames[2] == original && resync == 1);
    services.load_individual = [&](List &) -> std::int32_t { resync = 9; throw 1; };
    try { load_no_name(list, state, services); assert(false); } catch (int) {}
    assert(list.filenames[0] == replacement && resync == 9);
    list.filenames.assign(3, original);
    list.entries.resize(1);
    list.filenames.resize(1);
    load_archive("EXPAND", &list, state, services);
    assert(list.entries.size() == 2 && list.filenames[0] == original && !list.filenames[1]);
    services.load_archive = [&](std::string_view) { return ArchiveResult{1,{}}; };
    load_archive("EMPTY", &list, state, services);
    assert(list.entries[0] == nullptr && list.entries[1] == nullptr && list.attributes[0] == 0);
    resync = 0;
    List empty{};
    load_archive("EMPTY_TARGET", &empty, state, services);
    assert(resync == 0);
    std::vector<unsigned> destroyed;
    state.cache.directories.emplace_back(new unsigned(1), [&](const void *p) {
        destroyed.push_back(*static_cast<const unsigned *>(p)); delete static_cast<const unsigned *>(p);
    });
    state.cache.archives.emplace_back(new unsigned(2), [&](const void *p) {
        destroyed.push_back(*static_cast<const unsigned *>(p)); delete static_cast<const unsigned *>(p);
    });
    state.cache.was_loading = true;
    resync = 1;
    check_cache(state);
    assert(destroyed.empty() && state.cache.was_loading);
    resync = 0;
    check_cache(state);
    assert((destroyed == std::vector<unsigned>{1,2}) && !state.cache.was_loading);
    list.entries.assign(1,&first);
    list.filenames.assign(1,original);
    Group group{{"CON_REGULAR", &list}, {{"CON_REGULAR", &list}}};
    Catalog catalog{&group,&group,nullptr};
    language = 1;
    calls.clear();
    services.load_archive = [&](std::string_view name) {
        assert(common == &list);
        if (calls.empty()) assert(regular[0] == nullptr);
        calls.emplace_back(name);
        return ArchiveResult{-1,{}};
    };
    load_regular(0,catalog,state,services);
    assert((calls == std::vector<std::string>{"CON_REGULAR","CON_REGULAR_E"}));
    assert(regular[0] == &list && regular[1] == nullptr);
    load_regular(2,catalog,state,services);
    assert(common == nullptr && regular[0] == &list);
    group.regular.assign(32,{"CON_REGULAR",&list});
    services.load_archive = [&](std::string_view) { return ArchiveResult{-1,{}}; };
    load_regular(0,catalog,state,services);
    assert(common == nullptr && regular[31] == &list);
    language = 0;
    list.filenames[0] = nullptr;
    try { load_no_name(list,state,services); assert(false); }
    catch (const Error &error) { assert(error.boundary() == Boundary::Filename); }
}

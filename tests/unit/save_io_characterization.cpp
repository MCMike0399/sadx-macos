// Native adaptation contracts frozen before the IO/list owner migration.
// Original instruction comparisons remain in unchanged verify_original_save_*.
#include "game/save_write.hpp"
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <new>
#include <sstream>
#include <vector>

// Fault injection is armed only around append, never inside a service callback.
namespace { int allocation_budget = -1; }
void *operator new(std::size_t size) {
    if (allocation_budget == 0) throw std::bad_alloc();
    if (allocation_budget > 0) --allocation_budget;
    if (auto *p = std::malloc(size ? size : 1)) return p;
    throw std::bad_alloc();
}
void operator delete(void *p) noexcept { std::free(p); }
void *operator new[](std::size_t size) { return ::operator new(size); }
void operator delete[](void *p) noexcept { ::operator delete(p); }

namespace {
namespace save = sadx::save;
namespace files = sadx::file_select;
std::ostringstream output;
std::uint32_t cohort = 0;
std::uint32_t next() { cohort ^= cohort << 13; cohort ^= cohort >> 17; cohort ^= cohort << 5; return cohort; }
void bytes(const save::EncodedData &data) {
    constexpr char hex[] = "0123456789abcdef";
    for (auto x : data) output << hex[x >> 4] << hex[x & 15];
    output << '\n';
}
struct Fault {};
struct File final : files::File {};
struct Writer final : save::WriteCalls {
    std::int32_t disabled = 0, phase = -1;
    std::uint8_t retry = 9, ok = 8, count = 5, ready = 3, exec = 4, alert = 5;
    std::uint8_t deleting = 0, first = 1, file_count = 1, selected = 7, complete = 9;
    char old_name[14] = "SonicDX42.snc", new_name[14] = "unformatted", alien[14] = "SonicDX73.snc";
    save::SaveNameState name{old_name};
    files::List list = std::make_shared<files::ListNode>();
    save::Data data{};
    std::shared_ptr<File> file = std::make_shared<File>();
    int mutation = 0, fail = 0, failure_after_mutation = 0;
    bool null_allocation = false, null_open = false;
    std::uint32_t items = 0;
    Writer() {
        if (cohort) { data.total_time=next(); data.checksum=next(); selected=next(); count=next(); complete=next(); }
        list->next = std::make_shared<files::ListNode>();
        list->next->file = files::FileInfo{"sonicdx01.snc", 0x123456789ULL};
    }
    save::WriteState state() {
        return {disabled,retry,ok,count,phase,ready,exec,alert,deleting,first,
                file_count,selected,complete,name,list,data};
    }
    int identity() const {
        return !name.value ? 0 : name.value == old_name ? 1 : name.value == new_name ? 2 : name.value == alien ? 3 : 4;
    }
    void snapshot(int event) {
        output << event << ':' << disabled << ',' << +retry << ',' << +ok << ',' << +count
               << ',' << phase << ',' << +ready << ',' << +exec << ',' << +alert << ','
               << +deleting << ',' << +first << ',' << +file_count << ',' << +selected
               << ',' << +complete << ',' << identity() << '\n';
        bytes(save::encode(data));
        // Retained allocation content is separate from the published pointer.
        for (char c : new_name) output << +static_cast<unsigned char>(c) << ',';
        output << '\n';
    }
    void event(int id) {
        snapshot(id);
        if (fail == id && !failure_after_mutation) throw Fault{};
        if (mutation == id) {
            retry = 31; ok = 32; count = 33; phase = 34; ready = 35; exec = 36;
            alert = 37; complete = 38; file_count = 39; selected = 40;
            data.total_time = 0x98760000U + static_cast<unsigned>(id);
            if (id == 1) deleting = 1; // later capture guard must be lazy
            if (id == 2) first = 0; // capture may select the existing-name path
            if (id == 3) name.value = alien; // release is followed by clear
            if (id == 8) name.value = alien; // close mutation survives success store
            snapshot(id + 100);
        }
        if (fail == id && failure_after_mutation) throw Fault{};
    }
    void ensure_directory() override { event(1); }
    std::shared_ptr<files::File> open_write(std::string_view path) override {
        output << "path:" << path << '\n'; event(6);
        return null_open ? nullptr : file;
    }
    std::uint32_t write_save(const std::shared_ptr<files::File> &f, const save::EncodedData &encoded) override {
        assert(f == file); output << "written:"; bytes(encoded); event(7); return items;
    }
    void reset_input() override { event(9); }
    void close_write(const std::shared_ptr<files::File> &f) override { assert(f == file); event(8); }
    void run(int id) {
        output << "writer:" << id << '\n';
        save::SaveNameCalls names{
            [&](const char *p) { assert(p == name.value); event(3); },
            [&](std::size_t size) { assert(size == 14); event(4); return null_allocation ? nullptr : new_name; },
            [&](char *p,std::string_view text) {
                assert(p == new_name); output << "copy:" << text << '\n'; event(5);
                std::memcpy(p,text.data(),text.size()); p[text.size()] = 0;
            }};
        auto view = state();
        try { const auto result = save::write_file(view,*this,names,[&] { event(2); }); output << "return:" << result << '\n'; }
        catch (const Fault &) { output << "fault\n"; }
        catch (const std::invalid_argument &e) { output << "invalid:" << e.what() << '\n'; }
        snapshot(99);
    }
};
struct Search final : files::Search {};
struct Reader final : files::SaveFileCalls {
    files::List &list;
    save::Data &live;
    std::vector<files::FileInfo> entries;
    std::shared_ptr<Search> search = std::make_shared<Search>();
    std::shared_ptr<File> file = std::make_shared<File>();
    std::size_t index = 0, prefix = 0;
    std::uint32_t items = 0;
    int fail = 0, event_count = 0;
    bool null_open = false;
    save::EncodedData replacement{};
    Reader(files::List &l, save::Data &d) : list(l), live(d) {
        save::Data valid{}; std::int16_t rings = 1; save::initialize(valid,rings);
        replacement = save::encode(valid);
    }
    void event(int id) {
        output << "read-event:" << id << ',' << index << ',' << bool(list) << '\n';
        bytes(save::encode(live));
        if (++event_count == fail) throw Fault{};
    }
    std::optional<files::FoundFile> find_first(std::string_view pattern) override {
        output << pattern << '\n'; event(1);
        if (entries.empty()) return {};
        return files::FoundFile{search,entries[0]};
    }
    std::optional<files::FileInfo> find_next(const std::shared_ptr<files::Search> &s) override {
        assert(s == search); event(2); ++index;
        if (index >= entries.size()) return {};
        return entries[index];
    }
    void close_search(const std::shared_ptr<files::Search> &s) override { assert(s == search); event(3); }
    std::shared_ptr<files::File> open_file(std::string_view path) override {
        output << path << '\n'; event(4); return null_open ? nullptr : file;
    }
    std::uint32_t read_save(const std::shared_ptr<files::File> &f, save::EncodedData &encoded) override {
        assert(f == file); event(5); std::copy_n(replacement.begin(),prefix,encoded.begin()); return items;
    }
    void close_file(const std::shared_ptr<files::File> &f) override { assert(f == file); event(6); }
};
void list_snapshot(const files::List &list) {
    int n = 0;
    for (auto p = list; p; p = p->next,++n) {
        output << "node:" << n << ',' << bool(p->file) << ',' << bool(p->next) << '\n';
        if (p->file) output << p->file->name << ':' << p->file->last_write_time << '\n';
    }
}
void lists() {
    files::List list;
    files::append_save_list(list,std::nullopt,{});
    auto sentinel = list;
    assert(!sentinel->file && !sentinel->next);
    try { files::append_save_list(list,std::nullopt,{}); assert(false); }
    catch (const files::Error &e) { assert(e.boundary() == files::Boundary::IndeterminateSentinel); }
    assert(list == sentinel && sentinel->next && !sentinel->next->file);
    output << "partial-append\n"; list_snapshot(list);
    files::free_save_list(list);
    assert(!list && !sentinel->next && !sentinel->file);
    for (int budget : {0,1}) {
        for (bool existing : {false,true}) {
            files::List head;
            if (existing) files::append_save_list(head,std::nullopt,{});
            const std::optional<std::string> name{std::string(128,'x')};
            output << "allocation:" << budget << ',' << existing << '\n';
            allocation_budget = budget;
            try { files::append_save_list(head,name,{"",7}); allocation_budget = -1; output << "ok\n"; }
            catch (const std::bad_alloc &) { allocation_budget = -1; output << "bad_alloc\n"; }
            list_snapshot(head);
        }
    }
    std::vector<files::List> retained;
    std::vector<int> destruction;
    for (int i = 0; i < 4; ++i) {
        files::List node(new files::ListNode{},[&,i](files::ListNode *p) {
            // Observe reset fields at actual final shared_ptr destruction.
            destruction.push_back(i); output << "destroy:" << i << ',' << bool(p->file) << ',' << bool(p->next) << '\n'; delete p;
        });
        if (i) node->file = files::FileInfo{"SonicDX01.snc",static_cast<std::uint64_t>(i)};
        if (!list) list = node; else retained.back()->next = node;
        retained.push_back(node);
    }
    files::free_save_list(list);
    assert(!list && destruction.empty());
    for (const auto &p : retained) { assert(!p->next && !p->file); }
    for (int i : {2,0,3,1}) retained[i].reset();
    assert((destruction == std::vector<int>{2,0,3,1}));
    int id = 0;
    for (std::size_t prefix : {0,1,2,3,4,100,1391,1392}) {
        for (std::uint32_t items : {0,1}) {
            for (const char *name : {"SonicDX01.snc","SonicDX00.snc","SonicDX257.snc","SonicDXAA.snc","short"}) {
                files::List head; save::Data live{}; live.total_time = 0x12345678;
                Reader calls(head,live); calls.entries.push_back({name,0xfedcba9876543210ULL}); calls.prefix = prefix; calls.items = items;
                output << "enumerate:" << id++ << ',' << prefix << ',' << items << '\n';
                try { auto n = files::count_save_files(head,live,calls); output << "count:" << +n << '\n'; }
                catch (const files::Error &e) { output << "boundary:" << static_cast<int>(e.boundary()) << '\n'; }
                list_snapshot(head); bytes(save::encode(live));
            }
        }
    }
    for (int fail = 0; fail <= 7; ++fail) {
        files::List head; save::Data live{}; Reader calls(head,live);
        calls.entries.push_back({"SonicDX01.snc",9}); calls.prefix = 1392; calls.items = 1; calls.fail = fail;
        output << "enumeration-fault:" << fail << '\n';
        try { auto n = files::count_save_files(head,live,calls); output << "count:" << +n << '\n'; }
        catch (const Fault &) { output << "fault\n"; }
        list_snapshot(head); bytes(save::encode(live));
    }
    for (int size : {245,246}) {
        files::List head; save::Data live{}; Reader calls(head,live);
        calls.entries.push_back({"SonicDX01"+std::string(size-9,'x'),8});
        output << "path-size:" << size << '\n';
        try { (void)files::count_save_files(head,live,calls); }
        catch (const files::Error &e) { output << "boundary:" << static_cast<int>(e.boundary()) << '\n'; }
    }
    { files::List head; save::Data live{}; Reader calls(head,live); calls.prefix=1392; calls.items=1;
      calls.entries.assign(100,{"SonicDX01.snc",1});
      assert(files::count_save_files(head,live,calls)==99); assert(calls.index==98);
      output << "limit99\n"; list_snapshot(head); }
    for (bool missing : {false,true}) {
        files::List head; save::Data live{}; Reader calls(head,live); calls.null_open = missing;
        std::int32_t disabled = 0; std::uint8_t selected = 0; std::int16_t rings = 37;
        calls.prefix=3; calls.items=0;
        files::LoadState state{disabled,selected,"SonicDX42.snc",head,live,rings};
        output << "load:" << missing << '\n';
        try { output << files::load_selected_save(state,calls) << '\n'; }
        catch (const files::Error &e) { output << "boundary:" << static_cast<int>(e.boundary()) << '\n'; }
        bytes(save::encode(live));
    }
}
}
int main(int argc,char **argv) {
    const bool reserved = argc == 3 && std::string(argv[1]) == "--root-cohort";
    if (reserved) cohort = static_cast<std::uint32_t>(std::stoul(argv[2]));
    int id = 0;
    for (int event = 0; event <= 9; ++event) {
        for (int fail_mode = 0; fail_mode < 3; ++fail_mode) {
            Writer w; w.mutation = event; w.fail = fail_mode ? event : 0;
            w.failure_after_mutation = fail_mode == 2; w.run(id++);
        }
    }
    for (int mode = 0; mode < 9; ++mode) {
        Writer w;
        if (mode == 0) w.disabled = -1;
        if (mode == 1) w.deleting = 1;
        if (mode == 2) w.file_count = 99;
        if (mode == 3) w.null_allocation = true;
        if (mode == 4) w.first = 0;
        if (mode == 5) w.null_open = true;
        if (mode == 6) w.count = 4;
        if (mode == 7) w.items = 1;
        if (mode == 8) {
            auto tail = w.list;
            for (int slot=2;slot<=99;++slot) {
                char name[14]; std::snprintf(name,sizeof(name),"SonicDX%02d.snc",slot);
                files::append_save_list(w.list,std::string(name),{name,0});
            }
        }
        w.run(id++);
    }
    lists();
    const auto text = output.str();
    if (reserved || (argc == 2 && std::string(argv[1]) == "--capture")) { std::cout << text; return 0; }
    std::uint64_t hash = 14695981039346656037ULL;
    for (unsigned char c : text) { hash ^= c; hash *= 1099511628211ULL; }
    std::cout << text.size() << ' ' << hash << '\n';
    // Filled from immutable pre-migration source; full stream is retained locally.
    assert(text.size() == 3521423U && hash == 16575667018217874169ULL);
    return 0;
}

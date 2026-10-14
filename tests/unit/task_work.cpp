#include "engine/task_work.hpp"

#include <array>
#include <cstring>
#include <iostream>
#include <stdexcept>

using namespace sadx;
template<class T> static void check(const T& v) { if (!static_cast<bool>(v)) throw std::runtime_error("task work assertion"); }
int main() {
    try {
        TaskWorkArena arena(0x20000000);
        check(arena.blocks().size() == 2 && arena.free_head() == 0);
        auto a = arena.allocate(1).work;
        auto b = arena.allocate(56).work;
        auto c = arena.allocate(64).work;
        check(a->data_offset() == 32 && b->data_offset() == 96 && c->data_offset() == 192);
        std::array<std::uint8_t,32> bytes{};
        check(static_cast<bool>(arena.read(a,0,bytes.data(),32)));
        for (auto byte : bytes) check(byte == 0xff); // allocation never zeroes
        check(static_cast<bool>(arena.fill(a,0,0x17,1)));
        check(arena.release(b) && arena.release(a)); // next and previous coalesce
        auto reused = arena.allocate(80).work;
        check(reused->data_offset() == 32);
        check(arena.read(reused,0,bytes.data(),1) && bytes[0] == 0x17);
        check(a->released() && arena.release(a).failure == TaskWorkFailure::released_work);
        check(arena.release(reused) && arena.release(c));
        check(arena.blocks().size() == 2 && arena.free_head() == 0);
        check(arena.allocate(0xffffffff).failure == TaskWorkFailure::size_wrap);
        auto exact = arena.allocate(TaskWorkArena::usable_bytes-32).work;
        check(exact && arena.free_head() == TaskWorkArena::usable_bytes);
        check(arena.allocate(1).failure == TaskWorkFailure::exhausted);
        check(static_cast<bool>(arena.release(exact)));
        auto parent = arena.allocate(64).work;
        auto nested = arena.allocate(16).work;
        check(arena.set_reference(parent,56,nested));
        check(arena.reference(parent,56).work == nested);
        check(arena.fill(parent,56,0,4));
        check(arena.reference(parent,56) && !arena.reference(parent,56).work);
        TaskWorkArena other;
        check(other.release(parent).failure == TaskWorkFailure::foreign_work);
        check(arena.read(parent,100,bytes.data(),1).failure == TaskWorkFailure::out_of_bounds);
        check(arena.read(parent,0,nullptr,1).failure == TaskWorkFailure::invalid_buffer);
        check(arena.set_reference_view(parent,0,parent,16));
        const auto self=arena.reference_view(parent,0);
        check(self && self.work==parent && self.byte_offset==16 && self.allocation_data_offset==parent->data_offset());
        check(arena.reference(parent,0).failure==TaskWorkFailure::interior_reference_requires_view);
        check(arena.set_reference_view(parent,0,nested,32).failure==TaskWorkFailure::out_of_bounds);
        check(arena.set_reference_view(parent,0,{},1).failure==TaskWorkFailure::out_of_bounds);
        check(arena.set_reference_view(parent,0,nested,1));
        std::weak_ptr<TaskWork> weak_nested=nested;
        check(arena.release(nested));nested.reset();
        check(weak_nested.expired()); // reference metadata never owns itself/target
        const auto gone=arena.reference_view(parent,0);
        check(gone.failure==TaskWorkFailure::released_work && !gone.work && gone.byte_offset==1 && gone.allocation_data_offset);
        check(arena.set_reference(parent,4,parent));
        check(arena.reference(parent,4).work==parent);
        check(arena.release(parent));parent.reset();
        TaskWorkArena unknown;
        auto first = unknown.allocate(32).work;
        auto second = unknown.allocate(32).work;
        auto third = unknown.allocate(32).work;
        check(unknown.release(second) && unknown.release(first));
        auto larger = unknown.allocate(64).work;
        check(larger && unknown.read(larger,32,bytes.data(),4).failure == TaskWorkFailure::unobserved_pointer_bytes);
        (void)third;
        TaskWorkArena transported(0x20000000);auto mirrored=transported.source_bytes();
        transported.set_metadata_writer([&](std::uint32_t at,std::optional<std::uint32_t> value){check(value);std::memcpy(mirrored.data()+at,&*value,4);});
        const auto transported_a=transported.allocate(56).work,transported_b=transported.allocate(64).work;
        check(transported.release(transported_a)&&transported.release(transported_b));check(mirrored==transported.source_bytes());
        std::uint32_t live_word=0x7fc12345,stored_word=0;
        transported.set_payload_transport([&](std::uint32_t,void* output,std::uint32_t count){if(count!=4)return false;std::memcpy(output,&live_word,4);return true;},[&](std::uint32_t,const void* input,std::uint32_t count){if(count!=4)return false;std::memcpy(&stored_word,input,4);return true;});
        auto live=transported.allocate(32).work;std::uint32_t observed=0;check(transported.read(live,0,&observed,4)&&observed==live_word);check(transported.write(live,0,&live_word,4)&&stored_word==live_word);check(transported.fill(live,0,0x7a,4)&&stored_word==0x7a7a7a7a);check(transported.read(live,0,&observed,8).failure==TaskWorkFailure::unobserved_live_field);
        check(transported.set_reference(live,0,live));live_word=0;check(transported.reference(live,0)&&!transported.reference(live,0).work);live_word=0x12345678;check(transported.reference(live,0).failure==TaskWorkFailure::unbound_reference);
        bool missing_pointer=false;TaskWorkArena without_provenance;
        without_provenance.set_metadata_writer([&](std::uint32_t,std::optional<std::uint32_t> value){if(!value)missing_pointer=true;});
        check(without_provenance.allocate(32));check(missing_pointer);
        std::cout << "Task work arena: source limits, first-fit/reuse, coalescing, exact fit, payload and native references passed.\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}

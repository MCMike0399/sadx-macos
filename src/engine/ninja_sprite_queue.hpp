#pragma once
#include "engine/ninja_render.hpp"
#include "engine/ninja_sprite.hpp"
#include "engine/ninja_texture.hpp"
#include <functional>
#include <memory>
#include <string_view>
#include <vector>
namespace sadx {
enum class NinjaQueueFailure {none,invalid_arena_state,size_wrap,unknown_overflow_flag,missing_logger,logger_unavailable,out_of_bounds,unobserved_bytes,unknown_queue_mode,unknown_header_fact,missing_arena,missing_sort_bias,missing_constants,missing_bucket_owner,unknown_bucket_state,missing_depth,fp_failed,foreign_record,already_linked,unknown_texture_list,unknown_texture_count,unknown_texture_array,texture_array_short,unknown_texture_reference,unknown_texture_filename,render_failed,missing_color_owner,null_texture_array,unknown_display_switch,unknown_input,unknown_pointer_domain,missing_matrix,coordinate_failed,missing_depth_bias,texture_reference_conflict};
class NinjaRenderQueueArena;
struct NinjaQueueBlock {
    std::uint32_t offset=0,requested=0,aligned=0;
private:
    friend class NinjaRenderQueueArena;
    const NinjaRenderQueueArena* owner_=nullptr;
    std::uint64_t allocation_=0;
};
struct NinjaQueueLoggerResult {bool completed;};
using NinjaQueueLogger=std::function<NinjaQueueLoggerResult(std::string_view)>;
struct NinjaQueueAllocationResult {
    NinjaQueueFailure failure=NinjaQueueFailure::none;
    std::uint32_t source_boundary=0;
    std::optional<NinjaQueueBlock> block;
    explicit operator bool()const noexcept{return failure==NinjaQueueFailure::none;}
};
// Caller owns the physical resource/profile; no arbitrary arena size or Win32
// pointer cast. Buffer bytes initially have unknown provenance.403d30 never
// zeroes newly allocated storage. Failed source logger retains cursor/facts.
class NinjaRenderQueueArena {
public:
    NinjaRenderQueueArena(std::vector<std::uint8_t>,std::uint32_t cursor,std::uint32_t end,std::optional<std::uint32_t> overflow_reported);
    NinjaRenderQueueArena(const NinjaRenderQueueArena&)=delete;
    NinjaRenderQueueArena& operator=(const NinjaRenderQueueArena&)=delete;
    NinjaRenderQueueArena(NinjaRenderQueueArena&&)=delete;
    NinjaRenderQueueArena& operator=(NinjaRenderQueueArena&&)=delete;
    NinjaQueueAllocationResult allocate(std::uint32_t,const NinjaQueueLogger&);
    NinjaQueueFailure write(const NinjaQueueBlock&,std::uint32_t,const void*,std::uint32_t);
    NinjaQueueFailure read(const NinjaQueueBlock&,std::uint32_t,void*,std::uint32_t)const;
    // NOTSA:403db0 borrows end-size without advancing either cursor/end. The
    // owned byte allocation stays stable through407ab0 full-zero resets.
    std::uint8_t *borrow_from_end(std::uint32_t size);
    // NOTSA: actual validated allocation storage for a typed native resource.
    std::uint8_t *mutable_block(const NinjaQueueBlock &);
    std::uint32_t cursor()const noexcept{return cursor_;}
    std::uint32_t end()const noexcept{return end_;}
    std::optional<std::uint32_t> overflow_reported()const noexcept{return overflow_reported_;}
    const std::vector<std::uint8_t>& bytes()const noexcept{return bytes_;}
    // Actual 407ab0 buffer half: cursor=(base+0x1f)&~0x1f relative to the
    // source allocation base, every byte of the buffer becomes a known zero and
    // the overflow-logged flag 3abd9c8 is cleared. The base address is a caller-supplied source fact used only for
    // that alignment. Outstanding blocks become foreign (never reused ids).
    NinjaQueueFailure reset_407ab0(std::uint32_t source_base_address);
private:
    std::vector<std::uint8_t> bytes_,known_;
    std::uint32_t cursor_,end_;
    std::uint64_t generation_base_=0;
    std::optional<std::uint32_t> overflow_reported_;
    std::vector<NinjaQueueBlock> allocations_;
};
struct NinjaTextureMemoryReference {
    std::shared_ptr<const NinjaTextureArchive> archive;
    std::size_t record_ordinal;
    // The same actual native runtime/upload owner is retained with the list;
    // missing ownership is not an invented successfully loaded texture.
    std::optional<std::shared_ptr<const void>> runtime_owner;
    // Actual loaded TEXMEM fields. Raw source pointer words are provenance,
    // never native GPU handles; runtime ownership keeps unknown/null distinct.
    std::optional<std::uint32_t> global_id_at_0,palette_id_at_4,format_flags_at_20;
    std::optional<std::uint32_t> source_surface_word_at_34;
    mutable std::optional<std::uint16_t> palette_version_at_42;
};
struct NinjaTextureListOwner {
    using Entries=std::vector<std::optional<std::shared_ptr<const NinjaTextureMemoryReference>>>;
    std::optional<std::uint32_t> count;
    std::optional<std::shared_ptr<Entries>> entries;
    std::optional<bool> first_filename_is_invalid_sentinel;
};
struct NinjaTextureListResult {
    NinjaQueueFailure failure=NinjaQueueFailure::none;
    std::uint32_t source_boundary=0;
    std::optional<bool> source_not_ready;
    explicit operator bool()const noexcept{return failure==NinjaQueueFailure::none;}
};
// Source403250, EAX list, no guessed values or collapsed unknown/null facts.
NinjaTextureListResult ninja_texture_list_gate(const std::optional<std::shared_ptr<NinjaTextureListOwner>>&);
struct NinjaQueuedSpriteInput {
    NinjaSpriteInput sprite;
    std::optional<std::shared_ptr<NinjaTextureListOwner>> texture_list;
    std::shared_ptr<const std::vector<NinjaSpriteFrame>> animation;
};
struct NinjaQueueRecord {
    NinjaQueueBlock block;
    std::shared_ptr<NinjaRenderQueueArena> arena;
    std::optional<NinjaQueuedSpriteInput> sprite;
    // weak next avoids sentinel/list cycles; the queue owner retains records.
    std::weak_ptr<NinjaQueueRecord> next;
    bool next_is_sentinel=false,linked=false;
    std::optional<std::size_t> bucket;
    // Source header+c capture is separate from the sprite packet's +48 list.
    // Raw bytes remain provenance; this is the live retained native resource.
    std::optional<std::shared_ptr<NinjaTextureListOwner>> texture_list_at_c;
};
using NinjaQueueRecordHandle=std::shared_ptr<NinjaQueueRecord>;
struct NinjaQueueBucketResult {
    NinjaQueueFailure failure=NinjaQueueFailure::none;
    std::vector<NinjaQueueRecordHandle> records;
    explicit operator bool()const noexcept{return failure==NinjaQueueFailure::none;}
};
struct NinjaQueueBucketFrontResult {
    NinjaQueueFailure failure=NinjaQueueFailure::none;
    NinjaQueueRecordHandle record;
    explicit operator bool()const noexcept{return failure==NinjaQueueFailure::none;}
};
struct NinjaSpriteQueueRuntime {
    const std::optional<std::int32_t>* display_switch_at_3b1117c=nullptr;
    std::optional<float>* depth_bias_at_3abd9b8=nullptr;
    NinjaRenderState* render_state=nullptr; // borrowed authoritative packed blend
    std::optional<std::uint32_t> mode_at_3ab98ac;
    std::optional<std::uint8_t> flag_at_3abdcfe;
    std::optional<std::uint32_t> captured_word_at_3abd950,captured_word_at_3d0f9c8,captured_word_at_3d0f840,captured_word_at_3d0f9c4;
    std::optional<std::array<float,4>> color_at_3abd9a8,color_at_3ab9864;
    std::optional<std::uint32_t> flag_at_3b12208;
    std::optional<float> sort_bias_at_3abd9c0;
    std::optional<float> bucket_bias_at_7dcb60;
    NinjaQueueLogger logger;
    std::optional<std::shared_ptr<NinjaTextureListOwner>> captured_texture_list_at_3abd950;
};
struct NinjaQueueResult {
    NinjaQueueFailure failure=NinjaQueueFailure::none;
    std::uint32_t source_boundary=0;
    NinjaQueueRecordHandle record;
    std::optional<std::size_t> bucket;
    std::optional<X87Failure> fp_failure;
    std::uint16_t fp_exceptions=0;
    std::optional<NinjaRenderResult> render;
    explicit operator bool()const noexcept{return failure==NinjaQueueFailure::none;}
};
enum class NinjaSpriteSubmissionRoute {not_drawn,queued,immediate_required};
struct NinjaSpriteSubmissionResult {
    NinjaQueueResult queue;
    NinjaSpriteSubmissionRoute route=NinjaSpriteSubmissionRoute::not_drawn;
    std::optional<NinjaTextureListResult> texture_gate;
    std::optional<CoordinateFailure> coordinate_failure;
    explicit operator bool()const noexcept{return static_cast<bool>(queue);}
};
class NinjaSpriteQueue {
public:
    explicit NinjaSpriteQueue(std::shared_ptr<NinjaRenderQueueArena>);
    NinjaSpriteQueue(const NinjaSpriteQueue&)=delete;
    NinjaSpriteQueue& operator=(const NinjaSpriteQueue&)=delete;
    NinjaSpriteQueue(NinjaSpriteQueue&&)=delete;
    NinjaSpriteQueue& operator=(NinjaSpriteQueue&&)=delete;
    // Actual40734a..40736c empty-sentinel writes. Old allocated records stay
    // owned; source list reachability reset does not invent cleanup/reclamation.
    void initialize_sort_buckets();
    // 407ab0 reuse of the arena: earlier records are no longer owned. Bucket
    // links stay stale until 407330 re-initializes the sentinels.
    void forget_records_after_arena_reset();
    // 408630 frees and reallocates the buffer: earlier records are released.
    void rebind_arena(std::shared_ptr<NinjaRenderQueueArena>);
    const std::shared_ptr<NinjaRenderQueueArena>& arena()const noexcept{return arena_;}
    NinjaQueueResult allocate_record(std::uint32_t size,std::uint8_t type,std::uint8_t flags,std::uint32_t eax_value,NinjaSpriteQueueRuntime&); //403f60
    NinjaQueueResult insert(const NinjaQueueRecordHandle&,float depth,std::uint32_t policy,NinjaSpriteQueueRuntime&,X87Context&); //403e80
    NinjaSpriteSubmissionResult submit(const std::optional<std::shared_ptr<const NinjaQueuedSpriteInput>>&,std::optional<bool> source_pointer_domain,std::int32_t frame_index,std::uint32_t sprite_flags,std::uint32_t queue_flags,NinjaSpriteQueueRuntime&,NinjaMatrixPrimitives*,X87Context&); //404760 queued route
    NinjaQueueBucketResult bucket_records(std::size_t)const;
    NinjaQueueBucketFrontResult bucket_front(std::size_t)const;
    bool owns_record(const NinjaQueueRecordHandle&)const noexcept;
private:
    struct Bucket {bool initialized=false;NinjaQueueRecordHandle head,tail;};
    std::array<Bucket,2048> buckets_;
    std::shared_ptr<NinjaRenderQueueArena> arena_;
    std::vector<NinjaQueueRecordHandle> records_;
};
// Actual404240/77ff90 composition: source depth commands then raw color copy
// into the same live geometry owner. No fake completion on missing renderer.
NinjaQueueResult ninja_prepare_immediate_sprite(std::uint8_t,NinjaSpriteQueueRuntime&,const std::array<std::uint32_t,8>*,NinjaSpriteProjection&);
} // namespace sadx

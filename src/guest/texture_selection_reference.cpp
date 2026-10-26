#include "engine/ninja_texture_selection.hpp"
#include "engine/texture_selection_internal.hpp"
namespace sadx {
namespace {
struct Boundary {};
struct Operation {
    NinjaTextureSelectionState &state;
    NinjaTextureSelectionRuntime &runtime;
    NinjaTextureSelectionResult result;
    std::shared_ptr<const NinjaTextureMemoryReference> memory;
    [[noreturn]] void stop(NinjaTextureSelectionFailure why, std::uint32_t site) {
        result.failure = why;
        result.source_boundary = site;
        throw Boundary{};
    }
    template <class T>
    const T &need(const std::optional<T> &value, NinjaTextureSelectionFailure why,
                  std::uint32_t site) {
        if (!value)
            stop(why, site);
        return *value;
    }
    NinjaTextureBindCommand surface(std::uint32_t site) {
        NinjaTextureBindCommand out;
        out.memory = memory;
        out.source_surface_word = memory->source_surface_word_at_34;
        out.surface_owner = memory->runtime_owner;
        if (out.source_surface_word) {
            out.surface_is_null = *out.source_surface_word == 0;
            if (out.surface_owner && out.surface_is_null != !(*out.surface_owner))
                stop(NinjaTextureSelectionFailure::surface_owner_conflict, site);
        } else if (out.surface_owner)
            out.surface_is_null = !*out.surface_owner;
        else
            stop(NinjaTextureSelectionFailure::unknown_surface, site);
        return out;
    }
    void bind(NinjaTextureBindCommand command, std::uint32_t load, std::uint32_t dereference,
              std::uint32_t call, std::uint32_t id) {
        command.device = need(runtime.render.device_at_3d128b0,
                              NinjaTextureSelectionFailure::unknown_device, load);
        if (!command.device)
            stop(NinjaTextureSelectionFailure::null_device, dereference);
        command.source_call = call;
        result.binds.push_back(command);
        if (!runtime.set_texture) {
            result.pending_bind = command;
            stop(NinjaTextureSelectionFailure::missing_renderer, call);
        }
        const auto receipt = runtime.set_texture(command);
        if (!receipt.completed) {
            result.pending_bind = command;
            stop(NinjaTextureSelectionFailure::renderer_unavailable, call);
        }
        result.ignored_hresults.push_back(receipt.original_hresult);
        state.global_id_at_3d0fdf4 = id;
        state.active_memory_at_3d129a0 = memory;
    }
    std::optional<std::shared_ptr<const NinjaTextureMemoryReference>>
    entry(const std::shared_ptr<NinjaTextureListOwner> &list, std::uint32_t offset,
          std::uint32_t load, std::uint32_t dereference) {
        const auto array = need(list->entries, NinjaTextureSelectionFailure::unknown_array, load);
        if (!array)
            stop(NinjaTextureSelectionFailure::null_array, dereference);
        result.array_byte_offset = offset;
        if (offset % 12)
            stop(NinjaTextureSelectionFailure::unsupported_array_word, dereference);
        const auto ordinal = std::size_t(offset / 12);
        result.selected_ordinal = ordinal;
        if (ordinal >= array->size())
            stop(NinjaTextureSelectionFailure::array_short, dereference);
        const auto &selected = (*array)[ordinal];
        if (!selected)
            stop(NinjaTextureSelectionFailure::unknown_memory, dereference);
        return selected;
    }
    using Memory = std::shared_ptr<const NinjaTextureMemoryReference>;
    using List = std::shared_ptr<NinjaTextureListOwner>;
    void require_memory(const Memory &selected) {
        memory = selected;
        if (!memory)
            stop(NinjaTextureSelectionFailure::null_memory, 0x78cf24);
    }
    std::uint32_t global_index(const Memory &m) {
        return need(m->global_id_at_0, NinjaTextureSelectionFailure::unknown_global_id, 0x78cf24);
    }
    std::uint32_t palette_global_index(const Memory &m) {
        return need(m->global_id_at_0, NinjaTextureSelectionFailure::unknown_global_id, 0x78cdd0);
    }
    std::uint32_t palette_format(const Memory &m) {
        return need(m->format_flags_at_20, NinjaTextureSelectionFailure::unknown_format, 0x78cdc9);
    }
    std::uint32_t format(const Memory &m) {
        return need(m->format_flags_at_20, NinjaTextureSelectionFailure::unknown_format, 0x78cf60);
    }
    std::uint32_t palette_bank(const Memory &m) {
        return need(m->palette_id_at_4, NinjaTextureSelectionFailure::unknown_palette_id, 0x78cdcc);
    }
    NinjaTextureBindCommand palette_surface(const Memory &) { return surface(0x78cde8); }
    NinjaTextureBindCommand surface(const Memory &) { return surface(0x78cf3b); }
    bool is_null(const NinjaTextureBindCommand &surface) { return surface.surface_is_null; }
    std::uint32_t palette_bank_at(std::size_t index) {
        if (!runtime.palette_table_at_3d11f80)
            stop(NinjaTextureSelectionFailure::unknown_palette_table, 0x78cdf8);
        return need((*runtime.palette_table_at_3d11f80)[index].id_at_0,
                    NinjaTextureSelectionFailure::unknown_palette_entry, 0x78cdf8);
    }
    std::uint16_t palette_version_at(std::size_t index) {
        result.palette_index = index;
        return need((*runtime.palette_table_at_3d11f80)[index].version_at_c,
                    NinjaTextureSelectionFailure::unknown_palette_version, 0x78ce37);
    }
    std::uint16_t memory_palette_version(const Memory &m) {
        return need(m->palette_version_at_42, NinjaTextureSelectionFailure::unknown_palette_version,
                    0x78ce3b);
    }
    void store_palette_version(const Memory &m, std::uint16_t revision) {
        m->palette_version_at_42 = revision;
    }
    auto palette_colors(std::size_t index) {
        return need((*runtime.palette_table_at_3d11f80)[index].colors_at_8,
                    NinjaTextureSelectionFailure::unknown_palette_colors, 0x78ce4e);
    }
    void require_palette_surface(const NinjaTextureBindCommand &surface) {
        if (surface.surface_is_null)
            stop(NinjaTextureSelectionFailure::null_surface, 0x78ce51);
    }
    [[noreturn]] void
    stop_palette_upload(const std::shared_ptr<const std::array<std::uint32_t, 256>> &) {
        stop(NinjaTextureSelectionFailure::palette_lock_unavailable, 0x78ce5a);
    }
    std::uint32_t cached_global_index(texture_selection::detail::BindingPath path) {
        if (path == texture_selection::detail::BindingPath::Direct)
            result.palette_changed = false;
        return need(state.global_id_at_3d0fdf4, NinjaTextureSelectionFailure::unknown_cache_id,
                    path == texture_selection::detail::BindingPath::Palette ? 0x78ce08 : 0x78cf33);
    }
    void bind(const Memory &, NinjaTextureBindCommand command, std::uint32_t id,
              texture_selection::detail::BindingPath path) {
        if (path == texture_selection::detail::BindingPath::Palette)
            bind(std::move(command), 0x78ce10, 0x78ce15, 0x78ce1b, id);
        else
            bind(std::move(command), 0x78cf42, 0x78cf48, 0x78cf4e, id);
    }
    std::uint32_t return_value(std::uint32_t index) {
        if (!runtime.return_table_at_389d458)
            stop(NinjaTextureSelectionFailure::missing_return_table, 0x78cf69);
        return (*runtime.return_table_at_389d458)[index];
    }
    void store_current_list(const List &list) { state.current_list_at_3d0fa24 = list; }
    List current_list() {
        return need(state.current_list_at_3d0fa24, NinjaTextureSelectionFailure::unknown_list,
                    0x77f440);
    }
    Memory entry(const List &list, std::uint32_t offset, bool indexed) {
        return *entry(list, offset, indexed ? 0x77f449 : 0x77f3da, indexed ? 0x77f44e : 0x77f3dc);
    }
    void store_requested_memory(const Memory &selected) {
        state.requested_memory_at_3ce7128 = selected;
    }
};
} // namespace

NinjaTextureSelectionResult ninja_select_cached_texture(
    const std::optional<std::shared_ptr<const NinjaTextureMemoryReference>> &reference,
    NinjaTextureSelectionState &state, NinjaTextureSelectionRuntime &runtime) {
    Operation textures{state, runtime, {}, {}};
    try {
        const auto memory =
            textures.need(reference, NinjaTextureSelectionFailure::unknown_memory, 0x78cf24);
        textures.result.outcome = texture_selection::detail::cache(textures, memory);
    } catch (Boundary) {
    }
    return textures.result;
}
NinjaTextureSelectionResult
ninja_select_texture_list(const std::optional<std::shared_ptr<NinjaTextureListOwner>> &reference,
                          NinjaTextureSelectionState &state,
                          NinjaTextureSelectionRuntime &runtime) {
    Operation textures{state, runtime, {}, {}};
    try {
        const auto list =
            textures.need(reference, NinjaTextureSelectionFailure::unknown_list, 0x77f3d0);
        textures.result.outcome = texture_selection::detail::select_list(textures, list);
    } catch (Boundary) {
    }
    return textures.result;
}
NinjaTextureSelectionResult ninja_select_texture_index(std::uint32_t index,
                                                       NinjaTextureSelectionState &state,
                                                       NinjaTextureSelectionRuntime &runtime) {
    Operation textures{state, runtime, {}, {}};
    try {
        textures.result.outcome = texture_selection::detail::select_index(textures, index);
    } catch (Boundary) {
    }
    return textures.result;
}
NinjaRenderResult ninja_prepare_texture_draw_states(NinjaRenderState &runtime) {
    NinjaRenderResult result;
    constexpr std::array<std::uint32_t, 3> loads{{0x78b840, 0x78b852, 0x78b867}};
    constexpr std::array<std::uint32_t, 3> calls{{0x78b84c, 0x78b861, 0x78b876}};
    try {
        texture_selection::detail::prepare_draw_states([&](std::size_t index, std::uint32_t state,
                                                           std::uint32_t value) {
            if (!runtime.device_at_3d128b0) {
                result.failure = NinjaRenderFailure::unknown_device;
                result.source_boundary = loads[index];
                throw Boundary{};
            }
            if (!*runtime.device_at_3d128b0) {
                result.failure = NinjaRenderFailure::null_device;
                result.source_boundary = loads[index] + 5;
                throw Boundary{};
            }
            NinjaRenderStateCommand command{*runtime.device_at_3d128b0, state, value, calls[index]};
            result.commands.push_back(command);
            if (!runtime.set_render_state) {
                result.failure = NinjaRenderFailure::missing_renderer;
                result.source_boundary = calls[index];
                result.pending_command = command;
                throw Boundary{};
            }
            const auto receipt = runtime.set_render_state(command);
            if (!receipt.completed) {
                result.failure = NinjaRenderFailure::renderer_unavailable;
                result.source_boundary = calls[index];
                result.pending_command = command;
                throw Boundary{};
            }
            result.ignored_hresults.push_back(receipt.original_hresult);
        });
    } catch (Boundary) {
    }
    return result;
}
} // namespace sadx

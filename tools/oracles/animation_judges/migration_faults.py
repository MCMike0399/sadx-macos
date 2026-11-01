"""Anchor adaptations for the same thirteen frozen animation fault intents.

Only private copies are mutated. Expected values, cases and tolerances stay in
unchanged judges or the reviewed full frozen instruction streams.
"""
ENGINE = "src/player/animation/animation_engine.hpp"
INITIALIZER = "src/player/animation/sonic_animation_initialize.hpp"
TABLE_ADAPTER = "src/guest/sonic_animation_table_reference.cpp"
TABLE_FAULTS = {
    "eager_validation": (TABLE_ADAPTER,
        "        animation::detail::initialize_sonic_rows(sink);",
        "        for (std::uint16_t check = 0; check < animation::kSonicRecipes.size(); ++check) {\n"
        "            const auto &recipe = animation::kSonicRecipes[check];\n"
        "            if (recipe.action_export && (!*source || recipe.action_index >= (*source)->size() || !(*source)->at(recipe.action_index)))\n"
        "                sink.fail(PlayerAnimationFailure::unknown_field, check);\n"
        "        }\n"
        "        animation::detail::initialize_sonic_rows(sink);"),
    "unavailable_to_null": (TABLE_ADAPTER,
        "            fail(PlayerAnimationFailure::unknown_field, row);",
        "            return AnimationResourceReference{};"),
    "row_bound_146": (INITIALIZER,
        "        publish_sonic_row(sink, index, recipe, action);\n        sink.complete_row();",
        "        if (index != 146) {\n            publish_sonic_row(sink, index, recipe, action);\n            sink.complete_row();\n        }"),
    "reordered_publication": (INITIALIZER,
        "    for (std::uint16_t index = 0; index < kSonicRecipes.size(); ++index) {",
        "    std::array<decltype(sink.null_action()), 147> prefetched;\n"
        "    for (std::uint16_t check = 0; check < kSonicRecipes.size(); ++check)\n"
        "        prefetched[check] = kSonicRecipes[check].action_export\n"
        "            ? sink.load_action(check, kSonicRecipes[check].action_index) : sink.null_action();\n"
        "    for (std::uint16_t index = 0; index < kSonicRecipes.size(); ++index) {"),
}
# Reordered publication also redirects the normal read to its prefetched value;
# the runner applies that second replacement explicitly and records both anchors.

MOTION_FAULTS = {
    "next_high_byte": (ENGINE,
        "auto next = static_cast<std::uint8_t>(\n                read_value(record(v, from, Phase::NextRow).next, Phase::NextRow));",
        "auto next = static_cast<std::uint16_t>(\n                read_value(record(v, from, Phase::NextRow).next, Phase::NextRow));", "unit"),
    "cached_alias": ("src/player/player_animation.cpp",
        "        return view.player.motion.speed.x;",
        "        { static thread_local std::optional<float> cached;\n"
        "          if (!cached) cached = view.player.motion.speed.x; return *cached; }", "unit"),
    "reordered_action_publication": (ENGINE,
        "        access.state(v).buffer = a;\n        const auto &selected =",
        "        const auto &selected =", "unit"),
    "declared_count_clamp": (ENGINE,
        "        const auto frame = read_value(access.state(v).frame, at);",
        "        PlayerAnimationKeyBytes bounded = *input;\n"
        "        bounded.byte_extent = std::min<std::size_t>(bounded.byte_extent,\n"
        "            std::uint64_t(read_value(n.counts[channel], at)) * 16);\n"
        "        input = &bounded;\n"
        "        const auto frame = read_value(access.state(v).frame, at);", "blend"),
    "normalized_raw_pointer_count": (ENGINE,
        "access.raw_position_count(n, Phase::RawPositionCount)",
        "static_cast<std::uint32_t>(read_value(n.keys[2], Phase::RawPositionCount).id.value_or(0))", "blend"),
    "spilled_vector_fraction": (ENGINE,
        "        if (channel == 1) {\n            const auto coefficient = (f).rounded();",
        "        f = ExactFloat(fp, f.rounded());\n"
        "        if (channel == 1) {\n            const auto coefficient = (f).rounded();", "blend"),
    "release_order": (ENGINE,
        "                for (std::uint32_t channel = 0; channel < (channels == 3 ? 3U : 2U); ++channel) {",
        "                for (std::uint32_t ordinal = 0; ordinal < (channels == 3 ? 3U : 2U); ++ordinal) {\n"
        "                    const auto channel = (channels == 3 ? 3U : 2U) - 1U - ordinal;", "unit"),
    "default_release_order": (ENGINE,
        "            require_arena(rt, Phase::FreeNodeData);",
        "            arena_result(rt.arena->release(ref->allocation), Phase::FreeAction);\n"
        "            require_arena(rt, Phase::FreeNodeData);", "unit"),
}
# Two ordering faults need a matching move rather than an extra publication/free.
EXTRA_REPLACEMENTS = {
    "reordered_action_publication": (
        "        const auto &object = read_value(selected.object, Phase::AllocationObject);",
        "        const auto &object = read_value(selected.object, Phase::AllocationObject);\n"
        "        access.state(v).buffer = a;"),
    "default_release_order": (
        "            arena_result(rt.arena->release(ref->allocation), Phase::FreeAction);\n"
        "            access.state(v).buffer",
        "            access.state(v).buffer"),
}

#include "apps/native_gamma_actor_assets.hpp"
#include "runtime/native_player_system.hpp"
#include "runtime/native_display.hpp"
#include "runtime/character_texture_assets.hpp"
#include "game/game_mode.hpp"
#include <iostream>

namespace {
void require(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}
void lower_callback(sadx::TaskScheduler &, const sadx::TaskHandle &) {
    throw std::runtime_error("unrecovered test callback invoked");
}
void check_binding(const std::filesystem::path &directory) {
    using namespace sadx;
    namespace gamma = gamma_selection;
    const auto pack = character_pack::CharacterPack::load(directory / "characters/sonic-selection.actions");
    native::PlayerSystem players;
    players.gamma_assets = std::make_unique<native::GammaActorAssets>(
        native::app::load_gamma_actor_assets(directory, pack));
    auto &assets = *players.gamma_assets;
    character_pack::SceneState scene(pack);
    native::bind_gamma_resources(players, scene, assets);
    require(assets.animation_table->records.size() == 91 &&
        assets.declarations[0].action.identity == assets.stand.identity &&
        assets.declarations[61].status == native::ActorResourceStatus::KnownNull &&
        assets.declarations[1].status == native::ActorResourceStatus::Unavailable &&
        !assets.declarations[1].boundary.empty(), "complete imported table preserves resource states");
    require(!assets.animation_table->records.at(1).action &&
        assets.animation_table->records.at(0).action->id == assets.stand.identity,
        "unavailable animation must not become a known-null or stand substitute");
    TaskScheduler scheduler;
    auto arena = std::make_shared<TaskWorkArena>();
    require(bool(scheduler.initialize_source_pool(arena, std::shared_ptr<void>{})), "actual task pool");
    const auto task = scheduler.create_element(7, 3, nullptr).task;
    require(bool(task), "actual preview task");
    player_initialization::establish_zeroed_primary(scheduler, task);
    player_initialization::establish_zeroed_motion(scheduler, task);
    const auto primary_owner = task->primary_state;
    const auto motion_owner = task->motion_state;
    auto pose = player_initialization::bind_primary_view(task);
    pose.id = 2;
    pose.player_index = 5;
    pose.status = 0x14; // Cleared by CCL_ClearInfo; excludes the earlier follow-floor guard.
    pose.position = {13, 27, -9};
    pose.rotation = {111, 222, 333};
    motion_owner->target_yaw = 16384;
    std::int32_t card_result = 0;
    std::uint32_t global_timer = 0, movie_number = 0;
    std::int16_t rings = 0;
    std::uint16_t game_mode = 0;
    bootstrap::State bootstrap({card_result, global_timer, rings, movie_number, game_mode});
    bootstrap.event_work[393] = 1;
    bootstrap.event_work[465] = 1;
    frame::ModeState timing;
    native::Display display(directory / "system", timing);
    auto texture_assets = native::app::load_assets(directory).character_textures;
    std::array<std::vector<regular_textures::List>, 9> character_lists;
    for (std::size_t character = 0; character < character_lists.size(); ++character) {
        auto &lists = character_lists[character];
        lists.resize(texture_assets[character].size());
        for (std::size_t index = 0; index < lists.size(); ++index)
            lists[index].entries.resize(texture_assets[character][index].texture_count);
    }
    std::array<std::int8_t, 9> loaded{};
    native::CharacterTextureViews texture_views(texture_assets, character_lists);
    const auto catalog = texture_views.catalog(loaded);
    require(catalog.loaded == loaded.data() && catalog.tables == texture_views.tables.data() &&
        catalog.tables[6].entries[0].textures == &character_lists[6][0] &&
        catalog.tables[6].entries[0].archive->data() == texture_assets[6][0].archive.data(),
        "ordinary texture catalog borrows exact counters, lists and archive text");
    std::uint8_t resync = 0;
    const std::int32_t language = 1;
    regular_textures::List *common = nullptr;
    std::array<regular_textures::List *, 32> regular{};
    const regular_textures::Localizations localizations{};
    regular_textures::State texture_state{resync, language, common, regular, localizations, {}, {}};
    std::vector<std::string> texture_calls;
    regular_textures::Services texture_services{
        [&](std::string_view name) {
            texture_calls.emplace_back(name);
            return regular_textures::ArchiveResult{1, display.output().load_pvm_archive(name)};
        },
        [&](regular_textures::List &list) { return display.output().load_individual_textures(list); },
        [](std::string_view message) { throw std::runtime_error(std::string(message)); }};
    const std::int16_t stage = 0, act = 0;
    gamma::Context context{players, scheduler, bootstrap, display, task, stage, act,
        lower_callback, lower_callback, lower_callback, catalog, &texture_state, &texture_services};
    bool stopped = false;
    try { gamma::run(context, task); }
    catch (const flow::UnrecoveredGameFlowCall &error) {
        require(std::string(error.what()) == "native_gamma_load_setup_resource",
                "next source boundary follows real cursor initialization");
        stopped = true;
    }
    require(stopped, "unrecovered setup must fail loudly");
    const auto player = players.registry.players[5];
    require(player && player->stage == player_initialization::Stage::Complete &&
        players.registry.tasks[5] == task && players.registry.primary[5] == primary_owner &&
        players.registry.motion[5] == motion_owner,
        "real scheduler/NativeAccess initialization retains canonical task and player owners");
    auto initialized = player_initialization::bind_initialized_player(player);
    require(initialized.join_table == &players.gamma_weld,
            "actual initialized player's join field borrows the one mutable table");
    require(players.gamma_weld.rows.size() == 5, "four Gamma rows and complete sentinel");
    for (std::size_t index = 0; index < 5; ++index) {
        const auto &declaration = assets.joins[index];
        const auto &join = players.gamma_weld.rows[index];
        require(join.object == (declaration.object ? &scene.objects.object(declaration.object) : nullptr) &&
            join.source_object == (declaration.source_object ?
                &scene.objects.object(declaration.source_object) : nullptr) &&
            !join.destination_object && !join.original_vertices && !join.vertex_pairs &&
            join.input_mode == declaration.input_mode && join.vertex_count == declaration.vertex_count,
            "every imported weld field binds canonical object identities, including sentinel");
    }
    players.gamma_weld.rows[0].source_depth = 37;
    require(initialized.join_table->rows[0].source_depth == 37,
            "player sees later table writes without a cached per-player copy");
    const auto extra_allocation = gamma::extra_work(context, player);
    require(extra_allocation && extra_allocation->requested_size() == 580 &&
        extra_allocation != player->auxiliary && players.gamma_work.size() == 1 &&
        players.gamma_work.front().allocation == extra_allocation,
        "cursor fields belong to the actual separate E102WK allocation");
    auto &extra = players.gamma_work.front();
    const std::array<const gamma_cursors::WireModel *, 2> cursors{&extra.cursor1, &extra.cursor2};
    for (std::size_t index = 0; index < cursors.size(); ++index) {
        const auto &cursor = *cursors[index];
        const auto &declaration = assets.cursors[index];
        require(cursor.points.storage == &scene.geometry.vector_words(declaration.points) &&
            cursor.colors.storage == &scene.geometry.color_words(declaration.colors) &&
            cursor.indices == &scene.geometry.indices(declaration.indices) &&
            cursor.point_count == 24 && !cursor.texture &&
            cursor.expanded_points->requested_size() == 288 &&
            cursor.expanded_colors->requested_size() == 96,
            "source templates borrow the same geometry and allocate real output buffers");
        for (unsigned vertex = 0; vertex < 24; ++vertex) {
            const auto source = static_cast<unsigned>((*cursor.indices)[vertex]);
            for (unsigned component = 0; component < 3; ++component) {
                std::uint32_t actual = 0;
                require(bool(arena->read(cursor.expanded_points, (vertex * 3 + component) * 4,
                                         &actual, 4)) &&
                    actual == (*cursor.points.storage)[source * 3 + component],
                    "bit-exact imported cursor point inverse");
            }
            std::uint32_t actual = 0;
            require(bool(arena->read(cursor.expanded_colors, vertex * 4, &actual, 4)) &&
                actual == (*cursor.colors.storage)[source], "bit-exact imported cursor color inverse");
        }
    }
    require(extra.cursor1.points.storage == extra.cursor2.points.storage &&
        extra.cursor1.indices == extra.cursor2.indices &&
        extra.cursor1.colors.storage != extra.cursor2.colors.storage &&
        extra.cursor1.expanded_points != extra.cursor2.expanded_points,
        "shared template inputs and distinct source allocations retain their actual identities");
    require(task->display == lower_callback && task->destructor == lower_callback && pose.mode == 1 &&
        players.gamma.line_points == players.gamma.line_vertices.data() &&
        players.gamma.line_colors == assets.line_colors.data() && players.gamma.line_point_count == 2 &&
        !players.gamma.line_texture && !players.current.player,
        "actual caller stores execute before setup boundary; captured current publication stays null");
    const auto document = native::app::read_metadata(directory / "characters/gamma-actor.json");
    NSMutableDictionary *corrupt = [document mutableCopy];
    NSMutableArray *rows = [document[@"joins"] mutableCopy];
    NSMutableDictionary *row = [rows[4] mutableCopy];
    row[@"input_mode"] = @4;
    rows[4] = row;
    corrupt[@"joins"] = rows;
    bool rejected = false;
    try { (void)native::app::gamma_joins(corrupt, pack); }
    catch (const std::runtime_error &) { rejected = true; }
    require(rejected, "corrupt sentinel rejected at content boundary before pointer publication");
    std::cout << "PASS: canonical Gamma weld store and real cursor allocations reach setup boundary\n";
}
} // namespace
int main(int argc, char **argv) {
    @autoreleasepool {
        try {
            if (argc != 2) throw std::runtime_error("expected imported asset directory");
            check_binding(argv[1]);
            return 0;
        } catch (const std::exception &error) {
            std::cerr << error.what() << '\n';
            return 1;
        }
    }
}

#include "runtime/sonic_model_assets.hpp"
#include <stdexcept>

namespace sadx::native {
SonicModelAssets::SonicModelAssets(const character_pack::CharacterPack &pack,
                                   SonicModelDeclarations declarations)
    : declarations_(std::move(declarations)) {
    if (!declarations_.attach || !declarations_.root || declarations_.collision.size() != 2)
        throw std::runtime_error("incomplete Sonic model declarations");
    (void)pack.object(declarations_.attach);
    (void)pack.object(declarations_.root);
    if (!declarations_.join_target)
        throw std::runtime_error("missing Sonic join target");
    (void)pack.object(declarations_.join_target);
    for (const auto id : declarations_.direct_ahead) {
        if (!id)
            throw std::runtime_error("missing Sonic direct-ahead object");
        (void)pack.object(id);
    }
    const auto validate = [&](const auto &rows) {
        for (std::size_t index = 0; index < rows.size(); ++index) {
            const auto &row = rows[index];
            if ((row.object == 0) != (index + 1 == rows.size()))
                throw std::runtime_error("invalid Sonic join terminator");
            for (auto id : {row.object, row.source, row.destination})
                if (id)
                    (void)pack.object(id);
            if (row.pairs > declarations_.pairs.size() ||
                (row.pairs && declarations_.pairs[row.pairs - 1].size() != 2U * row.count))
                throw std::runtime_error("invalid Sonic vertex pairs");
            if (row.count && row.mode >= 1 && row.mode <= 3 && !row.pairs)
                throw std::runtime_error("missing Sonic vertex pairs");
        }
    };
    validate(declarations_.joins);
    validate(declarations_.metal_joins);
}
void SonicModelAssets::bind_scene(character_pack::ObjectState &objects) {
    if (objects_)
        throw std::runtime_error("Sonic model scene already bound");
    const auto bind = [&](const auto &rows) {
        PlayerJoinTable table;
        for (const auto &row : rows) {
            PlayerJoinVertex vertex{};
            if (row.object)
                vertex.object = &objects.object(row.object);
            if (row.source)
                vertex.source_object = &objects.object(row.source);
            if (row.destination)
                vertex.destination_object = &objects.object(row.destination);
            vertex.vertex_count = row.count;
            vertex.input_mode = row.mode;
            vertex.source_depth = row.source_depth;
            vertex.destination_depth = row.destination_depth;
            if (row.pairs)
                vertex.vertex_pairs = &declarations_.pairs[row.pairs - 1];
            table.rows.push_back(std::move(vertex));
        }
        return table;
    };
    (void)objects.object(declarations_.attach);
    (void)objects.object(declarations_.root);
    state_.joins = bind(declarations_.joins);
    state_.metal_joins = bind(declarations_.metal_joins);
    state_.direct_ahead = declarations_.direct_ahead;
    objects_ = &objects;
}
character_pack::ObjectState &SonicModelAssets::objects() const {
    if (!objects_)
        throw std::runtime_error("Sonic model scene is unavailable");
    return *objects_;
}
} // namespace sadx::native

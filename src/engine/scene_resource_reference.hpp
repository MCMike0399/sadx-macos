#pragma once
#include <cstddef>
#include <optional>
namespace sadx {
// Native loader identity, never an executable address cast. An absent outer
// optional is unknown; a present reference with no ID is observed null.
struct SceneResourceReference { std::optional<std::size_t> id; };
}

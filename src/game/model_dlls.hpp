#pragma once
#include "game/executable_image.hpp"
#include "cpu/source_bytes.hpp"
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
namespace sadx {
// Model DLLs bound by the static initializer 7c4e90 -> 401ac0 (this = 3ab9170)
// before WinMain. Each slot is LoadLibraryA(name); on success every listed
// export is GetProcAddress'd into its global (0 when absent).
struct ModelDllBinding {std::uint32_t slot;const char* name;std::uint32_t global;};
// Loaded images in 401ac0 slot order (null: LoadLibraryA returned NULL), each
// mapped at a distinct base with its relocations applied, plus the HMODULE
// identity the game stores. Reads go through whichever image maps the address.
struct ModelDllSet {
    std::array<std::shared_ptr<const ExecutableImage>,7> images;
    std::array<std::uint32_t,7> handles{};
    std::optional<std::uint32_t> u32(std::uint32_t va)const;
};
// Native loader policy: the images all prefer 0x10000000 (as do the mod loader
// and the Input mod), so the reference relocates them; here they are mapped at
// ascending 64 KiB-aligned bases from `first_base` in slot order.
ModelDllSet map_model_dlls(const std::array<std::shared_ptr<const ExecutableImage>,7>& preferred,std::uint32_t first_base,
    const std::array<std::uint32_t,7>& handles,std::string* error=nullptr);
struct ModelDllBindings {
    SourceBytes<0x3ab9170,28> handles_at_3ab9170;
    std::array<std::optional<std::uint32_t>,85> values; // kModelDllBindings order
    std::optional<std::uint32_t> value_at(std::uint32_t global)const;
};
enum class ModelDllFailure {none,forwarded_export};
struct ModelDllResult {
    ModelDllFailure failure=ModelDllFailure::none;std::uint32_t source_boundary=0;std::string export_name;
    explicit operator bool()const noexcept{return failure==ModelDllFailure::none;}
};
ModelDllResult bind_model_dlls_401ac0(const ModelDllSet&,ModelDllBindings&);
// Transcribed from the 401ac0 instruction bytes (push name / call GetProcAddress /
// mov [global],eax); verified against original instructions by
// tools/verify_original_model_dlls.py.
// owner-table: ModelDllBindings
constexpr std::array<const char*,7> kModelDllNames{{"SYSTEM\\chrmodels.dll","SYSTEM\\adv00models.dll","SYSTEM\\adv01models.dll","SYSTEM\\adv01Cmodels.dll","SYSTEM\\adv02models.dll","SYSTEM\\adv03models.dll","SYSTEM\\bosschaos0models.dll"}};
constexpr std::array<ModelDllBinding,85> kModelDllBindings{{
    {0,"___AMY_MODELS",0x38f6edc},
    {0,"___AMY_OBJECTS",0x38f6e70},
    {0,"___AMY_ACTIONS",0x3aad0e0},
    {0,"___AMY_MOTIONS",0x38f6ee0},
    {0,"___E102_OBJECTS",0x3a8af6c},
    {0,"___E102_MATERIALS",0x3aad0ec},
    {0,"___E102_ACTIONS",0x3ab916c},
    {0,"___E102_MOTIONS",0x38fcf04},
    {0,"___KNUCKLES_MODELS",0x3a7cf18},
    {0,"___KNUCKLES_OBJECTS",0x3a7cf1c},
    {0,"___KNUCKLES_MOTIONS",0x3aad100},
    {0,"___KNUCKLES_ACTIONS",0x3a7cf28},
    {0,"___BIG_MODELS",0x3aad118},
    {0,"___BIG_OBJECTS",0x38f6e98},
    {0,"___BIG_ACTIONS",0x3aad104},
    {0,"___BIG_MOTIONS",0x3aad0f8},
    {0,"___BIG_POINTS",0x3aad10c},
    {0,"___SONIC_MODELS",0x38f6ee8},
    {0,"___SONIC_OBJECTS",0x38f6e80},
    {0,"___SONIC_ACTIONS",0x38f6e7c},
    {0,"___SONIC_MOTIONS",0x3a8af70},
    {0,"___MILES_MODELS",0x3a8b08c},
    {0,"___MILES_OBJECTS",0x3aad124},
    {0,"___MILES_ACTIONS",0x3a7cf20},
    {0,"___MILES_MOTIONS",0x3ab9164},
    {1,"___LANDTABLESS",0x3aad0fc},
    {1,"___ADV00SS00_OBJECTS",0x38f6ee4},
    {1,"___ADV00SS01_OBJECTS",0x3aad128},
    {1,"___ADV00SS02_OBJECTS",0x38f6e84},
    {1,"___ADV00SS03_OBJECTS",0x3aad0f4},
    {1,"___ADV00SS04_OBJECTS",0x3a7cf40},
    {1,"___ADV00SS05_OBJECTS",0x38f6eec},
    {1,"___MODEL_SS_PEOPLE_OBJECTS",0x3aad11c},
    {1,"___MODEL_SS_PEOPLE_MOTIONS",0x38f6ea4},
    {1,"___ADV00_TEXLISTS",0x38f6ed0},
    {2,"___LANDTABLEEC",0x38f6e78},
    {2,"___ADV01EC00_OBJECTS",0x3aad138},
    {2,"___ADV01EC01_OBJECTS",0x38f6e8c},
    {2,"___ADV01EC02_OBJECTS",0x3a7cf08},
    {2,"___ADV01EC03_OBJECTS",0x3aad114},
    {2,"___ADV01EC04_OBJECTS",0x3a7cf14},
    {2,"___ADV01EC05_OBJECTS",0x3aad12c},
    {2,"___ADV01_OBJECTS",0x38f6ea8},
    {2,"___ADV01_TEXLISTS",0x38f6eac},
    {2,"___ADV01_ACTIONS",0x38fcef8},
    {2,"___ADV01_MODELS",0x38fcf00},
    {2,"___ADV01_POINTS",0x38f6eb4},
    {2,"___ADV01_MOTIONS",0x3a7cf0c},
    {2,"___ADV01_CACTIONS",0x3a8b07c},
    {3,"___LANDTABLEEC",0x38f6e88},
    {3,"___ADV01EC30_OBJECTS",0x3a8b094},
    {3,"___ADV01EC31_OBJECTS",0x38f6e9c},
    {3,"___ADV01EC32_OBJECTS",0x3aad0e4},
    {3,"___ADV01EC33_OBJECTS",0x3a8b090},
    {3,"___ADV01EC34_OBJECTS",0x3aad108},
    {3,"___ADV01EC35_OBJECTS",0x3a8b084},
    {3,"___ADV01C_OBJECTS",0x38f6ebc},
    {3,"___ADV01C_MOTIONS",0x3a7cf24},
    {3,"___ADV01C_MODELS",0x3aad0e8},
    {3,"___ADV01C_TEXLISTS",0x38f6ec8},
    {3,"___ADV01C_ACTIONS",0x3a7cf2c},
    {3,"___ADV01C_CACTIONS",0x3aad0f0},
    {4,"___LANDTABLEMR",0x3aad130},
    {4,"___ADV02MR00_OBJECTS",0x38f6e94},
    {4,"___ADV02MR01_OBJECTS",0x38fcefc},
    {4,"___ADV02MR02_OBJECTS",0x3ab9160},
    {4,"___ADV02MR03_OBJECTS",0x3a7cf34},
    {4,"___ADV02_OBJECTS",0x38f6ec4},
    {4,"___ADV02_MOTIONS",0x38f6eb0},
    {4,"___ADV02_MODELS",0x3aad120},
    {4,"___ADV02_ACTIONS",0x38f6ed4},
    {4,"___ADV02_TEXLISTS",0x38f6ef0},
    {5,"___LANDTABLEPAST",0x3aad110},
    {5,"___ADV03PAST00_OBJECTS",0x3a8af68},
    {5,"___ADV03PAST01_OBJECTS",0x3a7cf30},
    {5,"___ADV03PAST02_OBJECTS",0x3a7cf38},
    {5,"___ADV03_OBJECTS",0x38f6ecc},
    {5,"___ADV03_MOTIONS",0x38f6ed8},
    {5,"___ADV03_ACTIONS",0x38f6e74},
    {5,"___ADV03_TEXLISTS",0x3aad134},
    {6,"___LANDTABLEBOSSCHAOS0",0x3ab9168},
    {6,"___BOSSCHAOS0LAND_OBJECTS",0x38f6ea0},
    {6,"___BOSSCHAOS0_OBJECTS",0x3a8b098},
    {6,"___BOSSCHAOS0_ACTIONS",0x3a7cf10},
    {6,"___BOSSCHAOS0_TEXLISTS",0x3a8b080},
}};
// end owner-table

} // namespace sadx

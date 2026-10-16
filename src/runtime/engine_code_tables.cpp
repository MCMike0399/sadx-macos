#include "runtime/engine_code_tables.hpp"

namespace sadx::runtime {
EngineCodeTables engine_code_tables() {
    using Target = BlendTarget;
    using Blend = BlendFactor;
    using Compare = DepthComparison;
    using Key = KeyTranslationKind;
    return {
        {Target::Source, Target::Destination},
        {Blend::Zero, Blend::Zero, Blend::DestinationAlpha, Blend::DestinationColor,
         Blend::InverseDestinationAlpha, Blend::InverseDestinationColor, Blend::InverseSourceAlpha,
         Blend::InverseSourceColor, Blend::SourceAlpha, Blend::SourceColor, Blend::One,
         Blend::Zero},
        {Compare::Never, Compare::Less, Compare::Equal, Compare::LessEqual, Compare::Greater,
         Compare::NotEqual, Compare::GreaterEqual, Compare::Always},
        {{
            {Key::Ignore, 0},    {Key::Key, 41},
            {Key::Key, 30},      {Key::Key, 31}, // 0..3
            {Key::Key, 32},      {Key::Key, 33},
            {Key::Key, 34},      {Key::Key, 35}, // 4..7
            {Key::Key, 36},      {Key::Key, 37},
            {Key::Key, 38},      {Key::Key, 39}, // 8..11
            {Key::Key, 45},      {Key::Key, 46},
            {Key::Key, 42},      {Key::Key, 43}, // 12..15
            {Key::Key, 20},      {Key::Key, 26},
            {Key::Key, 8},       {Key::Key, 21}, // 16..19
            {Key::Key, 23},      {Key::Key, 28},
            {Key::Key, 24},      {Key::Key, 12}, // 20..23
            {Key::Key, 18},      {Key::Key, 19},
            {Key::Ignore, 0},    {Key::Ignore, 0}, // 24..27
            {Key::Key, 40},      {Key::Modifier, 1},
            {Key::Key, 4},       {Key::Key, 22}, // 28..31
            {Key::Key, 7},       {Key::Key, 9},
            {Key::Key, 10},      {Key::Key, 11}, // 32..35
            {Key::Key, 13},      {Key::Key, 14},
            {Key::Key, 15},      {Key::Key, 51}, // 36..39
            {Key::Key, 52},      {Key::Ignore, 0},
            {Key::Modifier, 2},  {Key::Key, 135}, // 40..43
            {Key::Key, 29},      {Key::Key, 27},
            {Key::Key, 6},       {Key::Key, 25}, // 44..47
            {Key::Key, 5},       {Key::Key, 17},
            {Key::Key, 16},      {Key::Key, 54}, // 48..51
            {Key::Key, 55},      {Key::Key, 56},
            {Key::Modifier, 32}, {Key::Key, 85}, // 52..55
            {Key::Modifier, 4},  {Key::Key, 44},
            {Key::Ignore, 0},    {Key::Key, 58}, // 56..59
            {Key::Key, 59},      {Key::Key, 60},
            {Key::Key, 61},      {Key::Key, 62}, // 60..63
            {Key::Key, 63},      {Key::Key, 64},
            {Key::Key, 65},      {Key::Key, 66}, // 64..67
            {Key::Key, 67},      {Key::Key, 83},
            {Key::Key, 71},      {Key::Key, 95}, // 68..71
            {Key::Key, 96},      {Key::Key, 97},
            {Key::Key, 86},      {Key::Key, 92}, // 72..75
            {Key::Key, 93},      {Key::Key, 94},
            {Key::Key, 87},      {Key::Key, 89}, // 76..79
            {Key::Key, 90},      {Key::Key, 91},
            {Key::Key, 98},      {Key::Ignore, 0}, // 80..83
            {Key::Ignore, 0},    {Key::Ignore, 0},
            {Key::Ignore, 0},    {Key::Key, 68}, // 84..87
            {Key::Key, 69},      {Key::Ignore, 0},
            {Key::Ignore, 0},    {Key::Ignore, 0}, // 88..91
            {Key::Ignore, 0},    {Key::Ignore, 0},
            {Key::Ignore, 0},    {Key::Ignore, 0}, // 92..95
            {Key::Ignore, 0},    {Key::Ignore, 0},
            {Key::Ignore, 0},    {Key::Ignore, 0}, // 96..99
            {Key::Ignore, 0},    {Key::Ignore, 0},
            {Key::Ignore, 0},    {Key::Ignore, 0}, // 100..103
            {Key::Ignore, 0},    {Key::Ignore, 0},
            {Key::Ignore, 0},    {Key::Ignore, 0}, // 104..107
            {Key::Ignore, 0},    {Key::Ignore, 0},
            {Key::Ignore, 0},    {Key::Ignore, 0}, // 108..111
            {Key::Ignore, 0},    {Key::Ignore, 0},
            {Key::Ignore, 0},    {Key::Ignore, 0}, // 112..115
            {Key::Ignore, 0},    {Key::Ignore, 0},
            {Key::Ignore, 0},    {Key::Ignore, 0}, // 116..119
            {Key::Ignore, 0},    {Key::Key, 138},
            {Key::Ignore, 0},    {Key::Key, 139}, // 120..123
            {Key::Ignore, 0},    {Key::Key, 137},
            {Key::Ignore, 0},    {Key::Ignore, 0}, // 124..127
            {Key::Ignore, 0},    {Key::Ignore, 0},
            {Key::Ignore, 0},    {Key::Ignore, 0}, // 128..131
            {Key::Ignore, 0},    {Key::Ignore, 0},
            {Key::Ignore, 0},    {Key::Ignore, 0}, // 132..135
            {Key::Ignore, 0},    {Key::Ignore, 0},
            {Key::Ignore, 0},    {Key::Ignore, 0}, // 136..139
            {Key::Ignore, 0},    {Key::Ignore, 0},
            {Key::Ignore, 0},    {Key::Ignore, 0}, // 140..143
            {Key::Key, 46},      {Key::Key, 47},
            {Key::Key, 52},      {Key::Ignore, 0}, // 144..147
            {Key::Ignore, 0},    {Key::Ignore, 0},
            {Key::Ignore, 0},    {Key::Ignore, 0}, // 148..151
            {Key::Ignore, 0},    {Key::Ignore, 0},
            {Key::Ignore, 0},    {Key::Ignore, 0}, // 152..155
            {Key::Key, 40},      {Key::Modifier, 16},
            {Key::Ignore, 0},    {Key::Ignore, 0}, // 156..159
            {Key::Ignore, 0},    {Key::Ignore, 0},
            {Key::Ignore, 0},    {Key::Ignore, 0}, // 160..163
            {Key::Ignore, 0},    {Key::Ignore, 0},
            {Key::Ignore, 0},    {Key::Ignore, 0}, // 164..167
            {Key::Ignore, 0},    {Key::Ignore, 0},
            {Key::Ignore, 0},    {Key::Ignore, 0}, // 168..171
            {Key::Ignore, 0},    {Key::Ignore, 0},
            {Key::Ignore, 0},    {Key::Ignore, 0}, // 172..175
            {Key::Ignore, 0},    {Key::Ignore, 0},
            {Key::Ignore, 0},    {Key::Key, 99}, // 176..179
            {Key::Ignore, 0},    {Key::Key, 84},
            {Key::Ignore, 0},    {Key::Key, 70}, // 180..183
            {Key::Modifier, 64}, {Key::Ignore, 0},
            {Key::Ignore, 0},    {Key::Ignore, 0}, // 184..187
            {Key::Ignore, 0},    {Key::Ignore, 0},
            {Key::Ignore, 0},    {Key::Ignore, 0}, // 188..191
            {Key::Ignore, 0},    {Key::Ignore, 0},
            {Key::Ignore, 0},    {Key::Ignore, 0}, // 192..195
            {Key::Ignore, 0},    {Key::Key, 72},
            {Key::Ignore, 0},    {Key::Key, 74}, // 196..199
            {Key::Key, 82},      {Key::Key, 75},
            {Key::Ignore, 0},    {Key::Key, 80}, // 200..203
            {Key::Ignore, 0},    {Key::Key, 79},
            {Key::Ignore, 0},    {Key::Key, 77}, // 204..207
            {Key::Key, 81},      {Key::Key, 78},
            {Key::Key, 73},      {Key::Key, 76}, // 208..211
            {Key::Ignore, 0},    {Key::Ignore, 0},
            {Key::Ignore, 0},    {Key::Ignore, 0}, // 212..215
            {Key::Ignore, 0},    {Key::Ignore, 0},
            {Key::Ignore, 0},    {Key::Ignore, 0}, // 216..219
            {Key::Ignore, 0},    {Key::Ignore, 0},
            {Key::Ignore, 0},    {Key::Ignore, 0}, // 220..223
            {Key::Ignore, 0},    {Key::Ignore, 0},
            {Key::Ignore, 0},    {Key::Ignore, 0}, // 224..227
            {Key::Ignore, 0},    {Key::Ignore, 0},
            {Key::Ignore, 0},    {Key::Ignore, 0}, // 228..231
            {Key::Ignore, 0},    {Key::Ignore, 0},
            {Key::Ignore, 0},    {Key::Ignore, 0}, // 232..235
            {Key::Ignore, 0},                      // 236..236
        }},
        {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1},
        {0.003921569f, 255.0f, 0.000015259022f, 0.0f, 1.0f},
        -1.0f,
        -65535.0f,
        {640.0f, 480.0f, 0.5f, 639.0f, 479.0f},
    };
}
} // namespace sadx::runtime

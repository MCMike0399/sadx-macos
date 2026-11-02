#include "guest/model_matrices_reference.hpp"
namespace sadx::model_matrices::reference {
FailureLocation locate(Failure failure) {
    switch (failure) {
    case Failure::MissingResource:
        return {0x4b81f0, "reached action/motion/object resource"};
    case Failure::MissingKeys:
        return {0x4b7d00, "reached node/channel/key word"};
    case Failure::MissingMotionData:
        return {0x4b823e, "motion data pointer word"};
    case Failure::MissingMotionType:
        return {0x4b824c, "motion type word"};
    case Failure::MissingMotionFrameCount:
        return {0x4b8254, "motion frame count"};
    case Failure::MissingActionObject:
        return {0x4b8280, "action root object reference"};
    case Failure::MissingMatrixStorage:
        return {0x4b82a9, "requested output bytes outside canonical 128-matrix storage"};
    case Failure::ZeroFrameCount:
        return {0x4b8265, "nonzero divisor (source division fault)"};
    case Failure::UnsupportedLayout:
        return {0x4b7d60, "two- or three-channel layout (source infinite loop)"};
    }
    return {0x4b81f0, "unknown model matrix failure"};
}
} // namespace sadx::model_matrices::reference

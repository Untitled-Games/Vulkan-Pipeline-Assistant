#ifndef RELOADFLAGS_H
#define RELOADFLAGS_H

#include "common.h"

namespace vpa {
    // Underlying bits for each individual part of the renderer
    enum class ReloadFlagBits : size_t {
        PIPELINE = 1,
        RENDER_PASS = 2,
        SHADERS = 4
    };

    inline constexpr size_t operator|(const ReloadFlagBits& f0, const ReloadFlagBits& f1) {
        return size_t(f0) | size_t(f1);
    }

    // Actual combinations specifying all parts that need reloaded for a particular part.
    enum class ReloadFlags : size_t {
        PIPELINE = size_t(ReloadFlagBits::PIPELINE), // 0001
        RENDER_PASS = ReloadFlagBits::RENDER_PASS | ReloadFlagBits::PIPELINE, // 0011
        SHADERS = ReloadFlagBits::SHADERS | ReloadFlagBits::PIPELINE, // 0101
        EVERYTHING = (ReloadFlagBits::SHADERS | ReloadFlagBits::RENDER_PASS) | size_t(ReloadFlagBits::PIPELINE) // 1111
    };

    inline constexpr bool operator&(const ReloadFlags& f0, const ReloadFlagBits& f1) {
        return size_t(f0) & size_t(f1);
    }
}

#endif // RELOADFLAGS_H

#ifndef RELOADFLAGS_H
#define RELOADFLAGS_H

#include "../common.h"

namespace vpa {
    // Underlying bits for each individual part of the renderer
    enum class ReloadFlagBits : size_t {
        Pipeline = 1,
        RenderPass = 2,
        Shaders = 4
    };

    inline constexpr size_t operator|(const ReloadFlagBits& f0, const ReloadFlagBits& f1) {
        return size_t(f0) | size_t(f1);
    }

    // Actual combinations specifying all parts that need reloaded for a particular part.
    enum class ReloadFlags : size_t {
        Pipeline = size_t(ReloadFlagBits::Pipeline), // 0001
        RenderPass = ReloadFlagBits::RenderPass | ReloadFlagBits::Pipeline, // 0011
        Shaders = ReloadFlagBits::Shaders | ReloadFlagBits::Pipeline, // 0101
        Everything = (ReloadFlagBits::Shaders | ReloadFlagBits::RenderPass) | size_t(ReloadFlagBits::Pipeline) // 1111
    };

    inline constexpr bool operator&(const ReloadFlags& f0, const ReloadFlagBits& f1) {
        return size_t(f0) & size_t(f1);
    }
}

#endif // RELOADFLAGS_H

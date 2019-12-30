#ifndef RELOADFLAGS_H
#define RELOADFLAGS_H

namespace vpa {
    enum class ReloadFlags : size_t {
        PIPELINE = 1, // 0001
        RENDER_PASS = 3, // 0011
        SHADERS = 5, // 0101
        DESCRIPTOR_VALUES = 8, // 1000
        EVERYTHING = 15 // 1111
    };

    inline bool operator&(const ReloadFlags& f0, const ReloadFlags& f1) {
        return size_t(f0) & size_t(f1);
    }
}

#endif // RELOADFLAGS_H

#ifndef MEMSIM_CORE_BLOCK_HPP
#define MEMSIM_CORE_BLOCK_HPP

#include <cstdint>

namespace memsim {

struct Block {
    int start;
    int size;
    std::uint64_t id; // Zero identifica um buraco.

    constexpr bool isFree() const noexcept { return id == 0; }

    // O limite é exclusivo. A soma usa um tipo mais largo que os endereços.
    constexpr std::int64_t getEnd() const noexcept
    {
        return static_cast<std::int64_t>(start) + size;
    }
};

} // namespace memsim

#endif

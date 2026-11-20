#ifndef MEMSIM_CORE_MEMORY_MANAGER_HPP
#define MEMSIM_CORE_MEMORY_MANAGER_HPP

#include "block.hpp"
#include "operation_result.hpp"

#include <cstdint>
#include <vector>

namespace memsim {

enum class FitAlg { First, Best, Worst, Next };
enum class DefragOption { Init, End, Opt };

class MemoryManager {
public:
    MemoryManager() = default;
    MemoryManager(const MemoryManager&) = default;
    MemoryManager& operator=(const MemoryManager&) = default;

    // Falhas preservam o estado inteiro, incluindo o ponteiro e a sequência de IDs.
    StepResult init(int size);
    StepResult alloc(int size, FitAlg algorithm);
    StepResult free_id(std::uint64_t id);
    StepResult free_addr(int addr);
    StepResult defrag(DefragOption option);
    StatsResult stats() const;

    // Capacidade zero representa o estado anterior ao primeiro init.
    bool initialized() const noexcept { return capacity_ > 0; }
    int capacity() const noexcept { return capacity_; }
    int next_pointer() const noexcept { return next_pointer_; }
    const std::vector<Block>& blocks() const noexcept { return blocks_; }

private:
    // Blocos de tamanho positivo, em ordem, cobrindo [0, capacity_).
    std::vector<Block> blocks_;
    int capacity_ = 0;
    int next_pointer_ = 0;
    // Testar o máximo antes de somar. Zero significa que nenhum ID foi emitido.
    std::uint64_t last_issued_id_ = 0;
};

} // namespace memsim

#endif

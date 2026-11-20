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
    StepResult freeId(std::uint64_t id);
    StepResult freeAddr(int addr);
    StepResult defrag(DefragOption option);
    StatsResult getStats() const;

    // Capacidade zero representa o estado anterior ao primeiro init.
    bool isInitialized() const noexcept { return capacity > 0; }
    int getCapacity() const noexcept { return capacity; }
    int getNextPointer() const noexcept { return next_pointer; }
    const std::vector<Block>& getBlocks() const noexcept { return blocks; }

private:
    bool hasValidState() const;

    // Blocos de tamanho positivo, em ordem, cobrindo [0, capacity).
    std::vector<Block> blocks;
    int capacity = 0;
    int next_pointer = 0;
    // Testar o máximo antes de somar. Zero significa que nenhum ID foi emitido.
    std::uint64_t last_issued_id = 0;
};

} // namespace memsim

#endif

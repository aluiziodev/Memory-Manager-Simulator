#include "memory_manager.hpp"

#include <algorithm>
#include <cassert>
#include <limits>
#include <unordered_set>

namespace memsim {

StepResult MemoryManager::init(int size)
{
    StepResult result;
    result.next_pointer_before = next_pointer;
    result.next_pointer_after = next_pointer;

    if (size <= 0) {   // Verifica se o tamanho é inválido.
        result.error = OperationError{ErrorCode::InvalidSize, std::nullopt};
        return result;
    }

    // Prepara o buraco antes de substituir o estado da sessão.
    std::vector<Block> initial_blocks{{0, size, 0}};
    blocks.swap(initial_blocks);
    capacity = size;
    next_pointer = 0;   // Reinicia o ponteiro de alocação.
    last_issued_id = 0;
    result.next_pointer_after = next_pointer;

    // Com as asserções habilitadas, um estado inválido interrompe a execução.
    assert(hasValidState());
    return result;
}

StepResult MemoryManager::alloc(int size, FitAlg algorithm)
{
    StepResult result;
    result.next_pointer_before = next_pointer;
    result.next_pointer_after = next_pointer;

    if (!isInitialized()) {
        result.error = OperationError{ErrorCode::NotInitialized, std::nullopt};
        return result;
    }
    if (size <= 0) {
        result.error = OperationError{ErrorCode::InvalidSize, std::nullopt};
        return result;
    }
    if (algorithm != FitAlg::First && algorithm != FitAlg::Best && algorithm != FitAlg::Worst) {
        result.error = OperationError{ErrorCode::UnsupportedAlgorithm, std::nullopt};
        return result;
    }

    auto chosen = blocks.end();
    int total_free = 0;
    int largest_hole = 0;
    for (auto block = blocks.begin(); block != blocks.end(); ++block) {
        if (!block->isFree()) {
            continue;
        }
        result.examined.push_back(*block);
        total_free += block->size;
        largest_hole = std::max(largest_hole, block->size);
        if (block->size < size) {
            continue;
        }

        // O percurso em ordem de endereço mantém o primeiro buraco em caso de empate.
        if (chosen == blocks.end() ||
            (algorithm == FitAlg::Best && block->size < chosen->size) ||
            (algorithm == FitAlg::Worst && block->size > chosen->size)) {
            chosen = block;
        }
        if (algorithm == FitAlg::First) {
            break;
        }
    }

    if (chosen == blocks.end()) {
        result.error = OperationError{ErrorCode::NoContiguousSpace,
                                      SpaceShortage{size, total_free, largest_hole}};
        return result;
    }
    result.chosen = *chosen;
    if (last_issued_id == std::numeric_limits<std::uint64_t>::max()) {
        result.error = OperationError{ErrorCode::IdExhausted, std::nullopt};
        return result;
    }

    const Block allocated{chosen->start, size, last_issued_id + 1};
    const int end = static_cast<int>(allocated.getEnd());
    if (chosen->size > size) {
        // A inserção pode realocar o vetor. Seu retorno aponta para a sobra.
        chosen = blocks.insert(chosen + 1, Block{end, chosen->size - size, 0}) - 1;
    }
    *chosen = allocated;
    last_issued_id = allocated.id;
    next_pointer = end;
    result.next_pointer_after = next_pointer;
    result.allocated = allocated;

    assert(hasValidState());
    return result;
}

StatsResult MemoryManager::getStats() const
{
    if (!isInitialized()) {
        return OperationError{ErrorCode::NotInitialized, std::nullopt};
    }

    Stats stats;
    stats.capacity = capacity;
    for (const Block& block : blocks) {
        if (block.isFree()) {
            stats.free += block.size;
            ++stats.hole_count;
            stats.largest_hole = std::max(stats.largest_hole, block.size);
        }
    }
    stats.occupied = capacity - stats.free;
    stats.usage_percent = 100.0 * stats.occupied / capacity;
    return stats;
}

StepResult MemoryManager::freeId(std::uint64_t id)
{
    const auto block = std::find_if(blocks.begin(), blocks.end(), [id](const Block& candidate) {
        return !candidate.isFree() && candidate.id == id;
    });
    return releaseBlock(block, ErrorCode::UnknownBlockId);
}

StepResult MemoryManager::freeAddr(int addr)
{
    const auto block = std::lower_bound(blocks.begin(), blocks.end(), addr,
                                       [](const Block& candidate, int address) {
                                           return candidate.start < address;
                                       });
    if (block == blocks.end() || block->start != addr || block->isFree()) {
        return releaseBlock(blocks.end(), ErrorCode::InvalidBlockStart);
    }
    return releaseBlock(block, ErrorCode::InvalidBlockStart);
}

StepResult MemoryManager::releaseBlock(std::vector<Block>::iterator block, ErrorCode missing_block_error)
{
    StepResult result;
    result.next_pointer_before = next_pointer;
    result.next_pointer_after = next_pointer;

    if (!isInitialized()) {
        result.error = OperationError{ErrorCode::NotInitialized, std::nullopt};
        return result;
    }
    if (block == blocks.end()) {
        result.error = OperationError{missing_block_error, std::nullopt};
        return result;
    }

    result.freed = *block;
    auto first = block;
    auto last = block + 1;
    if (first != blocks.begin() && (first - 1)->isFree()) {
        --first;
        result.merged_from.push_back(*first);
    }
    if (last != blocks.end() && last->isFree()) {
        result.merged_from.push_back(*last);
        ++last;
    }

    // Registra a coalescência antes de substituir os blocos no vetor.
    const int merged_size = static_cast<int>((last - 1)->getEnd() - first->start);
    const Block hole{first->start, merged_size, 0};
    if (!result.merged_from.empty()) {
        result.merged_into = hole;
    }
    *first = hole;
    blocks.erase(first + 1, last);

    assert(hasValidState());
    return result;
}

bool MemoryManager::hasValidState() const // Verifica a consistência do estado interno.
{
    if (capacity == 0) {
        return blocks.empty() && next_pointer == 0 && last_issued_id == 0;
    }
    if (capacity < 0 || next_pointer < 0 || next_pointer > capacity) {
        return false;
    }

    std::int64_t expected_start = 0;
    bool previous_was_free = false;
    // Registra os IDs ativos para detectar duplicatas.
    std::unordered_set<std::uint64_t> active_ids;
    for (const Block& block : blocks) {
        if (block.start != expected_start || block.size <= 0 || block.getEnd() > capacity) {
            return false;
        }

        if (block.isFree()) {
            if (previous_was_free) {
                return false;
            }
        } else if (block.id > last_issued_id || !active_ids.insert(block.id).second) {
            return false;
        }

        expected_start = block.getEnd();
        previous_was_free = block.isFree();
    }

    return expected_start == capacity;
}

} // namespace memsim

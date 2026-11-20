#include "memory_manager.hpp"

#include <cassert>
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

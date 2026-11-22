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

    if (!isInitialized()) { // Erro: memória não inicializada.
        result.error = OperationError{ErrorCode::NotInitialized, std::nullopt};
        return result;
    }
    if (size <= 0) {    // Erro: tamanho inválido.
        result.error = OperationError{ErrorCode::InvalidSize, std::nullopt};
        return result;
    }
    if (algorithm != FitAlg::First && algorithm != FitAlg::Best && algorithm != FitAlg::Worst 
        && algorithm != FitAlg::Next) { // Só admite os 4 válidos.
        result.error = OperationError{ErrorCode::UnsupportedAlgorithm, std::nullopt};
        return result;
    }

    /* Iteradores dos buracos, em ordem de endereço. Nada altera o vetor durante a busca,
    então os iteradores continuam válidos até a alocação. */

    std::vector<std::vector<Block>::iterator> order;
    for (auto block = blocks.begin(); block != blocks.end(); ++block) {
        if (block->isFree()) {
            order.push_back(block);
        }
    }

    if (algorithm == FitAlg::Next && !order.empty()) {
        /* Começa no primeiro buraco com fim > ponteiro. Se o ponteiro estiver dentro de
        um buraco, ele entra inteiro e a alocação começa no início dele. Se estiver
        depois de todos os buracos (inclusive == capacidade), recomeça do primeiro. */
        auto start = std::find_if(order.begin(), order.end(), [&](const auto& hole) {
            return hole->getEnd() > next_pointer;
        });
        if (start == order.end()) {
            start = order.begin();
        }
        std::rotate(order.begin(), start, order.end()); // lista circular, uma volta só
    }

    auto chosen = blocks.end();
    int total_free = 0;
    int largest_hole = 0;
    for (const auto& block : order) {
        result.examined.push_back(*block);
        total_free += block->size;
        largest_hole = std::max(largest_hole, block->size);
        if (block->size < size) {
            continue;
        }

        // Em empate, fica o primeiro examinado (em Best/Worst, o de menor endereço).
        if (chosen == blocks.end() ||
            (algorithm == FitAlg::Best && block->size < chosen->size) ||
            (algorithm == FitAlg::Worst && block->size > chosen->size)) {
            chosen = block;
        }
        if (algorithm == FitAlg::First || algorithm == FitAlg::Next) {
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

StepResult MemoryManager::defrag(DefragOption option)
{
    StepResult result;
    result.next_pointer_before = next_pointer;
    result.next_pointer_after = next_pointer;

    if (!isInitialized()) {
        result.error = OperationError{ErrorCode::NotInitialized, std::nullopt};
        return result;
    }
    if (option != DefragOption::Init && option != DefragOption::End &&
        option != DefragOption::Opt) {
        result.error = OperationError{ErrorCode::UnsupportedAlgorithm, std::nullopt};
        return result;
    }

    std::vector<Block> active;
    for (const Block& block : blocks) {
        if (!block.isFree()) {
            active.push_back(block);
        }
    }
    const std::size_t m = active.size();

    // packed_start[i]: endereço do bloco i se todos até ele ficarem colados no início.
    std::vector<int> packed_start(m + 1, 0);
    for (std::size_t i = 0; i < m; ++i) {
        packed_start[i + 1] = packed_start[i] + active[i].size;
    }
    const int used = packed_start[m];
    // Endereço do bloco i se ele e os seguintes ficarem colados no fim.
    const auto packed_end = [&](std::size_t i) { return capacity - (used - packed_start[i]); };

    // Corte k: os k primeiros blocos vão para o início e os demais para o fim.
    // prefix_cost[k] e suffix_cost[k] acumulam bytes movidos, então cada k custa O(1).
    std::vector<int> prefix_cost(m + 1, 0);
    std::vector<int> suffix_cost(m + 1, 0);
    for (std::size_t i = 0; i < m; ++i) {
        prefix_cost[i + 1] = prefix_cost[i] +
            (active[i].start != packed_start[i] ? active[i].size : 0);
    }
    for (std::size_t i = m; i-- > 0;) {
        suffix_cost[i] = suffix_cost[i + 1] +
            (active[i].start != packed_end(i) ? active[i].size : 0);
    }

    std::size_t cut = 0;
    if (option == DefragOption::End) {
        cut = m; // blocos no início, espaço livre no fim
    } else if (option == DefragOption::Opt) {
        int best_cost = prefix_cost[0] + suffix_cost[0];
        for (std::size_t k = 1; k <= m; ++k) {
            const int cost = prefix_cost[k] + suffix_cost[k];
            if (cost <= best_cost) { // <=: em empate vence o maior k
                best_cost = cost;
                cut = k;
            }
        }
    } // Init: cut = 0, blocos colados no fim, espaço livre no início

    std::vector<Block> layout;
    for (std::size_t i = 0; i < m; ++i) {
        if (i == cut && used < capacity) {
            layout.push_back(Block{packed_start[i], capacity - used, 0});
        }
        const int target = i < cut ? packed_start[i] : packed_end(i);
        layout.push_back(Block{target, active[i].size, active[i].id});
        if (target != active[i].start) {
            result.moved.push_back({active[i].id, active[i].start, target, active[i].size});
            result.moved_bytes += active[i].size;
        }
    }
    if (cut == m && used < capacity) {
        layout.push_back(Block{used, capacity - used, 0});
    }

    blocks.swap(layout);
    next_pointer = 0; // zerado em todos os modos
    result.next_pointer_after = next_pointer;

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

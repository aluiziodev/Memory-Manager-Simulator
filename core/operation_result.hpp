#ifndef MEMSIM_CORE_OPERATION_RESULT_HPP
#define MEMSIM_CORE_OPERATION_RESULT_HPP

#include "block.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <variant>
#include <vector>

namespace memsim {

struct Stats {
    int capacity = 0;
    int occupied = 0;
    int free = 0;
    std::size_t hole_count = 0;
    int largest_hole = 0;
    double usage_percent = 0.0;
};

enum class ErrorCode {
    NotInitialized,
    InvalidSize,
    UnknownBlockId,
    InvalidBlockStart,
    NoContiguousSpace,
    IdExhausted,
};

struct SpaceShortage {
    int requested;
    int total_free;
    int largest_hole;
};

struct OperationError {
    ErrorCode code;
    // Preenchido para NoContiguousSpace, com valores do estado anterior ao pedido.
    std::optional<SpaceShortage> space_shortage;
};

struct MovedBlock {
    std::uint64_t id;
    int from;
    int to;
    int size;
};

struct StepResult {
    std::optional<OperationError> error;
    int next_pointer_before = 0;
    int next_pointer_after = 0;

    // Cópias dos buracos anteriores à alocação, na ordem em que foram examinados.
    std::vector<Block> examined;
    std::optional<Block> chosen;
    std::optional<Block> allocated;

    std::optional<Block> freed;
    std::vector<Block> merged_from; // Buracos vizinhos absorvidos na liberação.
    std::optional<Block> merged_into; // Presente quando houve coalescência.

    std::vector<MovedBlock> moved;
    int moved_bytes = 0; // Número de blocos movidos = moved.size().

    bool ok() const noexcept { return !error.has_value(); }
};

using StatsResult = std::variant<Stats, OperationError>;

} // namespace memsim

#endif

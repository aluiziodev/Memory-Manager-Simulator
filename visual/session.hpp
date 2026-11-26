#ifndef MEMSIM_VISUAL_SESSION_HPP
#define MEMSIM_VISUAL_SESSION_HPP

#include "script/command.hpp"

#include <cstddef>
#include <istream>
#include <optional>
#include <variant>
#include <vector>

namespace memsim::visual {

// Controlador sem Qt: cada entrada guarda o estado completo após um comando.
class Session {
public:
    using LoadResult = std::variant<std::monostate, script::ParseError>;

    LoadResult load(std::istream& input);
    bool advance();
    bool back();
    void restart();

    bool hasScript() const noexcept { return !commands_.empty(); }
    bool canAdvance() const noexcept { return step() < commands_.size(); }
    bool canBack() const noexcept { return step() > 0; }
    std::size_t step() const noexcept { return history_.size() - 1; }
    std::size_t total() const noexcept { return commands_.size(); }
    const MemoryManager& memory() const noexcept { return history_.back().memory; }
    const script::Command* currentCommand() const noexcept;
    const script::CommandResult* currentResult() const noexcept;

private:
    struct Snapshot {
        MemoryManager memory;
        std::optional<script::CommandResult> result;
    };

    std::vector<script::Command> commands_;
    std::vector<Snapshot> history_{{MemoryManager{}, std::nullopt}};
};

} 

#endif

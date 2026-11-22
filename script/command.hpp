#ifndef MEMSIM_SCRIPT_COMMAND_HPP
#define MEMSIM_SCRIPT_COMMAND_HPP

#include "core/memory_manager.hpp"

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace memsim::script {

struct Init { int size; };
struct Alloc { int size; FitAlg algorithm; };
struct FreeId { std::uint64_t id; };
struct FreeAddr { int address; };
struct Show {};
struct ReadStats {};
struct Defrag { DefragOption option; };

using CommandArgs = std::variant<Init, Alloc, FreeId, FreeAddr, Show, ReadStats, Defrag>;

struct SourceLine {
    std::size_t number; // A primeira linha é 1.
    std::string text;
};

struct Command {
    CommandArgs args;
    SourceLine source;
};

struct IgnoredLine {}; // Linha vazia ou comentário.
struct ParseError {
    SourceLine source;
    std::string message;
};

using ParseResult = std::variant<Command, IgnoredLine, ParseError>;

// Representação por valor para preservar o resultado entre os passos.
struct MemoryView {
    std::vector<Block> blocks;
    Stats stats;
};

using CommandResult = std::variant<StepResult, MemoryView, Stats, OperationError>;

// show e stats são comandos válidos. O filtro visual é aplicado após a análise sintática.
ParseResult parseLine(std::string_view text, std::size_t line_number);
// Erros sem linha de origem (entrada vazia ou falha de leitura) usam número 0.
using VisualScriptResult = std::variant<std::vector<Command>, ParseError>;
VisualScriptResult readVisualScript(std::istream& input);

// Mutações retornam StepResult. Erros de show/stats retornam OperationError.
CommandResult execute(MemoryManager& memory, const Command& command);

} // namespace memsim::script

#endif

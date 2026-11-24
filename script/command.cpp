#include "command.hpp"

#include <array>
#include <charconv>
#include <istream>
#include <utility>

namespace memsim::script {
namespace {

std::string lowercase(std::string_view token)
{
    std::string result(token);
    for (char& c : result) {
        if (c >= 'A' && c <= 'Z') {
            c = static_cast<char>(c - 'A' + 'a');
        }
    }
    return result;
}

template <typename Number>
bool parseNumber(std::string_view token, Number minimum, Number& number)
{
    if (token.empty()) {
        return false;
    }
    const char* end = token.data() + token.size();
    const auto parsed = std::from_chars(token.data(), end, number);
    return parsed.ec == std::errc{} && parsed.ptr == end && number >= minimum;
}

} // namespace

ParseResult parseLine(std::string_view text, std::size_t line_number)
{
    constexpr std::string_view whitespace = " \t\r\n\v\f";
    auto start = text.find_first_not_of(whitespace);
    if (start == text.npos || text[start] == '#') {
        return IgnoredLine{};
    }

    const auto error = [&](const char* message) -> ParseResult {
        return ParseError{{line_number, std::string(text)}, message};
    };
    const auto command = [&](CommandArgs args) -> ParseResult {
        return Command{args, {line_number, std::string(text)}};
    };

    std::array<std::string_view, 3> tokens;
    std::size_t count = 0;
    while (start != text.npos) {
        if (count == tokens.size()) {
            return error("Argumentos em excesso.");
        }
        const auto end = text.find_first_of(whitespace, start);
        tokens[count++] = text.substr(start, end == text.npos ? text.npos : end - start);
        start = end == text.npos ? text.npos : text.find_first_not_of(whitespace, end);
    }

    const auto name = lowercase(tokens[0]);
    if (name == "show" || name == "stats") {
        if (count != 1) {
            return error("A consulta não aceita argumentos.");
        }
        return name == "show" ? command(Show{}) : command(ReadStats{});
    }
    if (name == "alloc") {
        int size;
        if (count != 3 || !parseNumber(tokens[1], 1, size)) {
            return error("Use alloc <tamanho positivo> <first|best|worst|next>.");
        }
        const auto option = lowercase(tokens[2]);
        if (option == "first") return command(Alloc{size, FitAlg::First});
        if (option == "best") return command(Alloc{size, FitAlg::Best});
        if (option == "worst") return command(Alloc{size, FitAlg::Worst});
        if (option == "next") return command(Alloc{size, FitAlg::Next});
        return error("Algoritmo de alocação inválido.");
    }
    if (name == "defrag") {
        if (count != 2) {
            return error("Use defrag <init|end|opt>.");
        }
        const auto option = lowercase(tokens[1]);
        if (option == "init") return command(Defrag{DefragOption::Init});
        if (option == "end") return command(Defrag{DefragOption::End});
        if (option == "opt") return command(Defrag{DefragOption::Opt});
        return error("Modo de compactação inválido.");
    }
    if (name == "init") {
        int size;
        if (count == 2 && parseNumber(tokens[1], 1, size)) {
            return command(Init{size});
        }
        return error("Use init <tamanho positivo>.");
    }
    if (name == "freeid") {
        std::uint64_t id;
        if (count == 2 && parseNumber(tokens[1], std::uint64_t{1}, id)) {
            return command(FreeId{id});
        }
        return error("Use freeid <ID positivo>.");
    }
    if (name == "freeaddr") {
        int address;
        if (count == 2 && parseNumber(tokens[1], 0, address)) {
            return command(FreeAddr{address});
        }
        return error("Use freeaddr <endereço não negativo>.");
    }
    return error("Comando desconhecido.");
}

CommandResult execute(MemoryManager& memory, const Command& command)
{
    if (const auto* init = std::get_if<Init>(&command.args)) {
        return memory.init(init->size);
    }
    if (const auto* alloc = std::get_if<Alloc>(&command.args)) {
        return memory.alloc(alloc->size, alloc->algorithm);
    }
    if (const auto* release = std::get_if<FreeId>(&command.args)) {
        return memory.freeId(release->id);
    }
    if (const auto* release = std::get_if<FreeAddr>(&command.args)) {
        return memory.freeAddr(release->address);
    }
    if (const auto* defrag = std::get_if<Defrag>(&command.args)) {
        return memory.defrag(defrag->option);
    }

    const auto stats = memory.getStats();
    if (const auto* error = std::get_if<OperationError>(&stats)) {
        return *error;
    }
    if (std::holds_alternative<Show>(command.args)) {
        return MemoryView{memory.getBlocks(), std::get<Stats>(stats)};
    }
    return std::get<Stats>(stats);
}

VisualScriptResult readVisualScript(std::istream& input)
{
    std::vector<Command> steps;
    std::string line;
    std::size_t number = 0;
    while (std::getline(input, line)) {
        auto parsed = parseLine(line, ++number);
        if (auto* error = std::get_if<ParseError>(&parsed)) {
            return std::move(*error);
        }
        auto* command = std::get_if<Command>(&parsed);
        if (!command || std::holds_alternative<Show>(command->args) ||
            std::holds_alternative<ReadStats>(command->args)) {
            continue;
        }
        if (steps.empty() && !std::holds_alternative<Init>(command->args)) {
            return ParseError{std::move(command->source), "O primeiro passo deve ser init."};
        }
        steps.push_back(std::move(*command));
    }
    if (input.bad() || !input.eof()) {
        return ParseError{{0, {}}, "Não foi possível ler o roteiro."};
    }
    if (steps.empty()) {
        return ParseError{{0, {}}, "O roteiro deve conter um comando init."};
    }
    return steps;
}

} // namespace memsim::script

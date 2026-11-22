#include "script/command.hpp"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <sstream>

using namespace memsim;
using namespace memsim::script;

namespace {

void check(bool condition, const char* description)
{
    if (!condition) {
        std::cerr << "Falhou: " << description << '\n';
        std::exit(EXIT_FAILURE);
    }
}

Command parseCommand(std::string_view text)
{
    auto result = parseLine(text, 3);
    const auto* command = std::get_if<Command>(&result);
    check(command && command->source.number == 3 && command->source.text == text,
          "comando com número e texto originais");
    return *command;
}

void testCommands()
{
    check(std::get<Init>(parseCommand(" \tINIT 32\r").args).size == 32, "init");
    check(std::get<Init>(parseCommand("init " + std::to_string(std::numeric_limits<int>::max())).args).size ==
              std::numeric_limits<int>::max(), "limite do tamanho");
    check(std::get<FreeId>(parseCommand("freeid 18446744073709551615").args).id ==
              std::numeric_limits<std::uint64_t>::max(), "limite do ID");
    check(std::get<FreeAddr>(parseCommand("FreeAddr 0").args).address == 0, "endereço zero");
    check(std::holds_alternative<Show>(parseCommand("SHOW").args), "show");
    check(std::holds_alternative<ReadStats>(parseCommand("stats").args), "stats");

    const char* fits[] = {"alloc 8 FIRST", "alloc 8 best", "alloc 8 worst", "alloc 8 next"};
    const FitAlg algorithms[] = {FitAlg::First, FitAlg::Best, FitAlg::Worst, FitAlg::Next};
    for (std::size_t i = 0; i < 4; ++i) {
        const auto alloc = std::get<Alloc>(parseCommand(fits[i]).args);
        check(alloc.size == 8 && alloc.algorithm == algorithms[i], fits[i]);
    }

    check(std::get<Defrag>(parseCommand("defrag INIT").args).option == DefragOption::Init, "defrag init");
    check(std::get<Defrag>(parseCommand("defrag end").args).option == DefragOption::End, "defrag end");
    check(std::get<Defrag>(parseCommand("defrag opt").args).option == DefragOption::Opt, "defrag opt");
}

void testSyntax()
{
    for (const auto* line : {"", " \t\r", "  # exemplo\r"}) {
        check(std::holds_alternative<IgnoredLine>(parseLine(line, 1)), "linha ignorável");
    }
    for (const auto* line : {"foo", "init", "init 0", "init -1", "alloc 10", "alloc 2 fastest",
                             "alloc 2 next extra", "freeid 0", "freeid -1", "freeid 18446744073709551616",
                             "freeaddr -1", "freeaddr 9999999999999999999999", "freeaddr 12x",
                             "show extra", "stats extra", "defrag middle", "defrag opt extra"}) {
        const auto result = parseLine(line, 7);
        const auto* error = std::get_if<ParseError>(&result);
        check(error && error->source.number == 7 && error->source.text == line, line);
    }
}

void testVisualScript()
{
    std::istringstream official(
        "# Exemplo oficial\r\ninit 32\r\nalloc 8 next\r\nalloc 8 next\r\nalloc 8 next\r\n"
        "freeid 1\r\nshow\r\nalloc 4 next\r\nalloc 6 next\r\nstats");
    const auto result = readVisualScript(official);
    const auto* steps = std::get_if<std::vector<Command>>(&result);
    check(steps && steps->size() == 7, "sete passos do roteiro oficial");
    check(steps->front().source.number == 2 && steps->back().source.number == 9 &&
          steps->back().source.text == "alloc 6 next\r", "origem dos passos filtrados");

    std::istringstream bad_query("init 32\n# comentário\nshow extra\n");
    check(std::get<ParseError>(readVisualScript(bad_query)).source.number == 3,
          "consulta inválida é rejeitada antes de ser filtrada");
    std::istringstream missing_init("stats\nalloc 8 next");
    check(std::get<ParseError>(readVisualScript(missing_init)).source.number == 2,
          "primeiro passo deve ser init");
    std::istringstream empty("  # comentário\nshow\n");
    check(std::holds_alternative<ParseError>(readVisualScript(empty)), "roteiro sem passos");
    std::istringstream executable("init 1\nalloc 8 next\ninit 32");
    const auto commands = readVisualScript(executable);
    check(std::get<std::vector<Command>>(commands).size() == 3,
          "erros de execução continuam como passos");
}

} // namespace

int main()
{
    testCommands();
    testSyntax();
    testVisualScript();
}

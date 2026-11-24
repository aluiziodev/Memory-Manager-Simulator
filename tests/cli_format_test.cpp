#include "cli/format.hpp"

#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <locale>
#include <sstream>

using namespace memsim;

namespace {

class CommaDecimal : public std::numpunct<char> {
    char do_decimal_point() const override { return ','; }
};

void expect(const std::string& actual, std::string_view expected)
{
    if (actual != expected) {
        std::cerr << "Esperado:\n" << expected << "\nObtido:\n" << actual;
        std::exit(EXIT_FAILURE);
    }
}

std::string run(MemoryManager& memory, std::initializer_list<std::string_view> lines)
{
    std::ostringstream output;
    output.imbue(std::locale(std::locale::classic(), new CommaDecimal));
    for (const auto line : lines) {
        const auto command = std::get<script::Command>(script::parseLine(line, 1));
        cli::writeResult(output, command, script::execute(memory, command));
    }
    return output.str();
}

} // namespace

int main()
{
    MemoryManager memory;
    const std::string stats =
        "== Estatísticas ==\n"
        "Tamanho total: 64 bytes\n"
        "Ocupado: 16 bytes | Livre: 48 bytes\n"
        "Buracos (fragmentação externa): 1\n"
        "Maior buraco livre: 48 bytes\n"
        "Uso efetivo: 25.00%\n";
    expect(run(memory, {"init 64", "alloc 10 first", "alloc 8 first", "freeid 2",
                        "alloc 6 best", "show"}),
           "Mapa de Memória (64 bytes)\n"
           "------------------------------------------------------------------\n"
           "[################................................................]\n"
           "[1111111111333333................................................]\n"
           "------------------------------------------------------------------\n"
           "\n"
           "Blocos ativos: [id=1] @0 +10B (usado=10B) | [id=3] @10 +6B (usado=6B)\n"
           + stats);
    expect(run(memory, {"stats"}), stats);

    expect(run(memory, {"init 32", "alloc 8 first", "alloc 8 first", "freeaddr 0",
                        "alloc 20 first"}),
           "Erro: sem espaço contíguo para 20 bytes (livre total: 24, maior buraco: 16)\n");

    struct DefragCase {
        std::string_view command;
        std::string_view expected;
    };
    const DefragCase defrag_cases[] = {
        {"defrag init", "Defrag INIT: 2 blocos movidos, 16 bytes deslocados\n"},
        {"defrag end", "Defrag END: 1 blocos movidos, 6 bytes deslocados\n"},
        {"defrag opt", "Defrag OPT: 1 blocos movidos, 6 bytes deslocados\n"},
    };
    for (const auto& test : defrag_cases) {
        expect(run(memory, {"init 64", "alloc 10 first", "alloc 8 first", "alloc 6 first",
                            "freeid 2", test.command}), test.expected);
    }

    expect(run(memory, {"init 10", "alloc 1 first", "alloc 1 first", "alloc 1 first",
                        "alloc 1 first", "alloc 1 first", "alloc 1 first", "alloc 1 first",
                        "alloc 1 first", "alloc 1 first", "alloc 1 first", "show"}),
           "Mapa de Memória (10 bytes)\n"
           "------------\n"
           "[##########]\n"
           "[1234567890]\n"
           "------------\n"
           "\n"
           "Blocos ativos: [id=1] @0 +1B (usado=1B) | [id=2] @1 +1B (usado=1B) | "
           "[id=3] @2 +1B (usado=1B) | [id=4] @3 +1B (usado=1B) | "
           "[id=5] @4 +1B (usado=1B) | [id=6] @5 +1B (usado=1B) | "
           "[id=7] @6 +1B (usado=1B) | [id=8] @7 +1B (usado=1B) | "
           "[id=9] @8 +1B (usado=1B) | [id=10] @9 +1B (usado=1B)\n"
           "== Estatísticas ==\n"
           "Tamanho total: 10 bytes\n"
           "Ocupado: 10 bytes | Livre: 0 bytes\n"
           "Buracos (fragmentação externa): 0\n"
           "Maior buraco livre: 0 bytes\n"
           "Uso efetivo: 100.00%\n");
}

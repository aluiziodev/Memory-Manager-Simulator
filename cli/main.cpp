#include "format.hpp"

#include <cstdio>
#include <iostream>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

int main()
{
#ifdef _WIN32
    const bool interactive = _isatty(_fileno(stdin)) != 0;
#else
    const bool interactive = isatty(STDIN_FILENO) != 0;
#endif

    memsim::MemoryManager memory;
    std::string line;
    std::size_t line_number = 0;
    while (true) {
        if (interactive) {
            std::cout << "> " << std::flush;
        }
        if (!std::getline(std::cin, line)) {
            break;
        }

        const auto parsed = memsim::script::parseLine(line, ++line_number);
        if (const auto* command = std::get_if<memsim::script::Command>(&parsed)) {
            memsim::cli::writeResult(std::cout, *command, memsim::script::execute(memory, *command));
        } else if (const auto* error = std::get_if<memsim::script::ParseError>(&parsed)) {
            std::cout << "Erro: linha " << error->source.number << ": " << error->message << '\n';
        }
    }
    if (std::cin.bad() || !std::cin.eof()) {
        std::cerr << "Erro: falha ao ler a entrada.\n";
        return 1;
    }
    return 0;
}

#include "visual/session.hpp"

#include <cstdlib>
#include <iostream>
#include <sstream>

using namespace memsim;
using namespace memsim::visual;

namespace {
void check(bool ok, const char* message)
{
    if (!ok) { std::cerr << message << '\n'; std::exit(EXIT_FAILURE); }
}

void sameState(const MemoryManager& a, const MemoryManager& b)
{
    check(a.getCapacity() == b.getCapacity() && a.getNextPointer() == b.getNextPointer(), "capacidade/ponteiro");
    const auto& x = a.getBlocks();
    const auto& y = b.getBlocks();
    check(x.size() == y.size(), "quantidade de blocos");
    for (std::size_t i = 0; i < x.size(); ++i)
        check(x[i].start == y[i].start && x[i].size == y[i].size && x[i].id == y[i].id, "blocos");
}
}

int main()
{
    Session s;
    check(!s.hasScript() && !s.canAdvance() && !s.canBack(), "sessao inicial");
    std::istringstream script("init 16\nalloc 8 next\nalloc 12 next\ninit 10\nalloc 2 next\n");
    check(std::holds_alternative<std::monostate>(s.load(script)) && s.total() == 5, "carregar");
    check(s.advance() && s.advance(), "dois passos");
    const auto beforeError = s.memory();
    check(s.advance() && s.step() == 3, "erro tambem e passo");
    const auto* failed = std::get_if<StepResult>(s.currentResult());
    check(failed && !failed->isOk() && s.currentCommand()->source.text == "alloc 12 next", "resultado do erro");
    sameState(s.memory(), beforeError);
    check(s.advance() && s.advance() && !s.canAdvance(), "segundo init e fim");
    check(s.memory().getBlocks()[0].id == 1 && s.memory().getNextPointer() == 2, "id e ponteiro apos reiniciar");
    check(s.back() && s.back() && s.step() == 3, "volta ate erro");
    sameState(s.memory(), beforeError);
    check(std::get<StepResult>(*s.currentResult()).error.has_value(), "mensagem restaurada");
    check(s.back() && s.advance() && s.advance() && s.advance(), "refazer passos");
    check(s.memory().getBlocks()[0].id == 1 && s.memory().getNextPointer() == 2, "refazer id/ponteiro");
    std::istringstream invalid("alloc 1 first\n");
    check(std::holds_alternative<script::ParseError>(s.load(invalid)) && s.step() == 5, "arquivo invalido preserva sessao");
    s.restart();
    check(s.step() == 0 && !s.memory().isInitialized() && !s.canBack(), "reiniciar");
    check(!s.back() && s.advance(), "limites");
}

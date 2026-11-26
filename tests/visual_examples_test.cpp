#include "visual/session.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace memsim;
using namespace memsim::visual;

namespace {
void check(bool condition, const char* description)
{
    if (!condition) { std::cerr << description << '\n'; std::exit(EXIT_FAILURE); }
}
void sameState(const MemoryManager& left, const MemoryManager& right)
{
    check(left.getCapacity() == right.getCapacity() && left.getNextPointer() == right.getNextPointer(), "estado escalar");
    const auto& a = left.getBlocks();
    const auto& b = right.getBlocks();
    check(a.size() == b.size(), "numero de blocos");
    for (std::size_t i = 0; i < a.size(); ++i)
        check(a[i].start == b[i].start && a[i].size == b[i].size && a[i].id == b[i].id, "blocos");
}
void run(const char* path, int capacity, int occupied, int holes, int pointer)
{
    std::ifstream file(path);
    check(file.good(), "arquivo de exemplo");
    Session session;
    check(std::holds_alternative<std::monostate>(session.load(file)), "roteiro valido");
    std::vector<MemoryManager> states{session.memory()};
    while (session.advance()) states.push_back(session.memory());
    check(session.step() == session.total(), "chegou ao fim");
    const auto stats = std::get<Stats>(session.memory().getStats());
    check(stats.capacity == capacity && stats.occupied == occupied &&
          static_cast<int>(stats.hole_count) == holes && session.memory().getNextPointer() == pointer,
          "estado final esperado");
    for (std::size_t i = states.size(); i-- > 1;) {
        check(session.back(), "voltar");
        sameState(session.memory(), states[i - 1]);
    }
    check(!session.canBack() && !session.memory().isInitialized(), "voltou ao inicio");
}
}

int main(int argc, char* argv[])
{
    if (argc != 5) return EXIT_FAILURE;
    run(argv[1], 32, 26, 2, 6);
    run(argv[2], 24, 20, 1, 0);
    run(argv[3], 64, 17, 4, 43);
    run(argv[4], 24, 8, 2, 24);
}

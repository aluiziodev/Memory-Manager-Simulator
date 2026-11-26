#include "visual/qt/memory_map.hpp"

#include <QApplication>

#include <cstdlib>

using namespace memsim;
using namespace memsim::visual;

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    MemoryManager memory;
    MemoryMap map;
    map.resize(832, 132); // 784 pixels para 4096 endereços.
    if (map.blockAt(24) || memory.init(4096).error || memory.alloc(1, FitAlg::First).error) return EXIT_FAILURE;
    map.setMemory(memory);
    const Block* first = map.blockAt(24);
    const Block* last = map.blockAt(807);
    if (!first || first->id != 1 || !last || !last->isFree()) return EXIT_FAILURE;
    if (MemoryMap::colorForId(1) != MemoryMap::colorForId(1) ||
        MemoryMap::colorForId(1) == MemoryMap::colorForId(2)) return EXIT_FAILURE;
    memory.init(8);
    memory.alloc(1, FitAlg::First);
    map.setMemory(memory);
    if (!map.blockAt(24) || map.blockAt(24)->id != 1 || !map.blockAt(807)->isFree()) return EXIT_FAILURE;
    if (map.grab().isNull()) return EXIT_FAILURE;
    return EXIT_SUCCESS;
}

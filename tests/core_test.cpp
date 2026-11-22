#include "core/memory_manager.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include <variant>

using namespace memsim;

namespace {

int failures = 0;

#define CHECK(cond)                                                                  \
    do {                                                                             \
        if (!(cond)) {                                                               \
            std::fprintf(stderr, "FALHOU %s:%d: %s\n", __FILE__, __LINE__, #cond);  \
            ++failures;                                                              \
        }                                                                            \
    } while (0)

MemoryManager started(int capacity)
{
    MemoryManager m;
    CHECK(m.init(capacity).isOk());
    return m;
}

void allocAll(MemoryManager& m, std::initializer_list<int> sizes, FitAlg alg)
{
    for (int s : sizes) {
        CHECK(m.alloc(s, alg).isOk());
    }
}

bool hasBlock(const MemoryManager& m, int start, int size, std::uint64_t id)
{
    const auto& blocks = m.getBlocks();
    return std::any_of(blocks.begin(), blocks.end(), [&](const Block& b) {
        return b.start == start && b.size == size && b.id == id;
    });
}

// Após compactar: exatamente 1 buraco se havia espaço livre, nenhum se estava cheia.
void checkSingleHole(const MemoryManager& m, int free_bytes)
{
    const Stats s = std::get<Stats>(m.getStats());
    CHECK(s.free == free_bytes);
    CHECK(s.hole_count == (free_bytes > 0 ? 1u : 0u));
}

// ---------- C04: Next Fit ----------

// Exemplo do enunciado: ponteiros 8, 16, 24, depois 28 e 6 (volta ao início).
void testNextFitOficial()
{
    MemoryManager m = started(32);
    for (int expected_after : {8, 16, 24}) {
        CHECK(m.alloc(8, FitAlg::Next).next_pointer_after == expected_after);
    }
    CHECK(m.freeId(1).isOk());

    StepResult r = m.alloc(4, FitAlg::Next);
    CHECK(r.isOk());
    CHECK(r.next_pointer_before == 24);
    CHECK(r.chosen && r.chosen->start == 24);
    CHECK(r.allocated && r.allocated->start == 24 && r.allocated->id == 4);
    CHECK(r.next_pointer_after == 28);

    r = m.alloc(6, FitAlg::Next); // não cabe em @28, retorna ao @0
    CHECK(r.isOk());
    CHECK(r.next_pointer_before == 28);
    CHECK(r.examined.size() == 2 && r.examined[0].start == 28 && r.examined[1].start == 0);
    CHECK(r.allocated && r.allocated->start == 0 && r.allocated->id == 5);
    CHECK(r.next_pointer_after == 6);

    const Stats s = std::get<Stats>(m.getStats());
    CHECK(s.occupied == 26 && s.free == 6 && s.hole_count == 2 && s.largest_hole == 4);
    CHECK(std::fabs(s.usage_percent - 81.25) < 1e-9);
}

// Ponteiro igual à capacidade e ponteiro dentro de um buraco.
void testNextNoLimiteEDentroDeBuraco()
{
    MemoryManager a = started(16);
    allocAll(a, {8, 8}, FitAlg::Next);
    CHECK(a.getNextPointer() == 16);
    CHECK(a.freeId(1).isOk());
    StepResult r = a.alloc(4, FitAlg::Next);
    CHECK(r.next_pointer_before == 16);
    CHECK(r.allocated && r.allocated->start == 0);
    CHECK(r.next_pointer_after == 4);

    MemoryManager b = started(16);
    allocAll(b, {8}, FitAlg::Next);
    CHECK(b.freeId(1).isOk()); // buraco único @0 +16, ponteiro 8 dentro dele
    r = b.alloc(12, FitAlg::Next);
    CHECK(r.next_pointer_before == 8);
    CHECK(r.allocated && r.allocated->start == 0);
    CHECK(r.next_pointer_after == 12);
}

// Cenário misto de Q01: First @0, Best @38, Worst @14, Next @48.
void testDivergencia()
{
    MemoryManager base = started(64);
    allocAll(base, {10, 4, 20, 4, 6, 4}, FitAlg::First);
    CHECK(base.getNextPointer() == 48);
    for (std::uint64_t id : {1, 3, 5}) {
        CHECK(base.freeId(id).isOk());
    }

    const struct { FitAlg alg; int start; } cases[] = {
        {FitAlg::First, 0}, {FitAlg::Best, 38}, {FitAlg::Worst, 14}, {FitAlg::Next, 48}};
    for (const auto& c : cases) {
        MemoryManager copy = base;
        StepResult r = copy.alloc(5, c.alg);
        CHECK(r.isOk());
        CHECK(r.allocated && r.allocated->start == c.start);
    }
}

// ---------- C05: compactação ----------

// INIT deixa o espaço livre no início, END no fim. OPT empata k=1 e k=2 e escolhe k=2.
void testDefragOficial()
{
    MemoryManager base = started(64);
    allocAll(base, {10, 8, 6}, FitAlg::First);
    CHECK(base.freeId(2).isOk());

    MemoryManager a = base;
    StepResult r = a.defrag(DefragOption::Init);
    CHECK(r.isOk() && r.moved.size() == 2 && r.moved_bytes == 16);
    CHECK(hasBlock(a, 48, 10, 1) && hasBlock(a, 58, 6, 3) && hasBlock(a, 0, 48, 0));
    CHECK(a.getNextPointer() == 0);
    checkSingleHole(a, 48);

    MemoryManager b = base;
    r = b.defrag(DefragOption::End);
    CHECK(r.moved.size() == 1 && r.moved_bytes == 6);
    CHECK(r.moved[0].id == 3 && r.moved[0].from == 18 && r.moved[0].to == 10);
    CHECK(hasBlock(b, 0, 10, 1) && hasBlock(b, 10, 6, 3) && hasBlock(b, 16, 48, 0));
    CHECK(b.getNextPointer() == 0);
    checkSingleHole(b, 48);

    MemoryManager c = base;
    r = c.defrag(DefragOption::Opt);
    CHECK(r.moved.size() == 1 && r.moved_bytes == 6);
    CHECK(hasBlock(c, 0, 10, 1) && hasBlock(c, 10, 6, 3) && hasBlock(c, 16, 48, 0));
    CHECK(c.getNextPointer() == 0);
    checkSingleHole(c, 48);
}

// Mínimo no interior: custos INIT 8 B, END 16 B, OPT 4 B.
void testOptNoInterior()
{
    MemoryManager base = started(24);
    allocAll(base, {4, 2, 4, 2, 12}, FitAlg::First);
    CHECK(base.freeId(2).isOk());
    CHECK(base.freeId(4).isOk());

    MemoryManager a = base;
    CHECK(a.defrag(DefragOption::Init).moved_bytes == 8);
    checkSingleHole(a, 4);

    MemoryManager b = base;
    CHECK(b.defrag(DefragOption::End).moved_bytes == 16);
    checkSingleHole(b, 4);

    MemoryManager c = base;
    StepResult r = c.defrag(DefragOption::Opt);
    CHECK(r.moved_bytes == 4 && r.moved.size() == 1);
    CHECK(r.moved[0].id == 3 && r.moved[0].from == 6 && r.moved[0].to == 4);
    CHECK(hasBlock(c, 0, 4, 1) && hasBlock(c, 4, 4, 3) && hasBlock(c, 8, 4, 0) &&
          hasBlock(c, 12, 12, 5));
    checkSingleHole(c, 4);
}

// Memória cheia, memória vazia e estado anterior ao init.
void testDefragBordas()
{
    MemoryManager nao_iniciada;
    CHECK(nao_iniciada.defrag(DefragOption::Opt).error->code == ErrorCode::NotInitialized);

    for (DefragOption o : {DefragOption::Init, DefragOption::End, DefragOption::Opt}) {
        MemoryManager cheia = started(8);
        allocAll(cheia, {8}, FitAlg::First);
        StepResult r = cheia.defrag(o);
        CHECK(r.isOk() && r.moved.empty() && r.moved_bytes == 0);
        CHECK(hasBlock(cheia, 0, 8, 1));
        CHECK(cheia.getNextPointer() == 0);
        checkSingleHole(cheia, 0);

        MemoryManager vazia = started(8);
        r = vazia.defrag(o);
        CHECK(r.isOk() && r.moved.empty());
        CHECK(hasBlock(vazia, 0, 8, 0));
        checkSingleHole(vazia, 8);
    }
}

} // namespace

int main()
{
    testNextFitOficial();
    testNextNoLimiteEDentroDeBuraco();
    testDivergencia();
    testDefragOficial();
    testOptNoInterior();
    testDefragBordas();

    if (failures == 0) {
        std::puts("Todos os testes passaram.");
        return 0;
    }
    std::fprintf(stderr, "%d verificacao(oes) falharam.\n", failures);
    return 1;
}
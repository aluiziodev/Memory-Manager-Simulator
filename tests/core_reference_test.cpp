#include "core/memory_manager.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <vector>

using namespace memsim;

namespace {
struct Region { int start; int size; std::uint64_t id; };

void check(bool condition, int seed, int step, const char* what)
{
    if (!condition) {
        std::fprintf(stderr, "seed=%d passo=%d: %s\n", seed, step, what);
        std::exit(EXIT_FAILURE);
    }
}

struct Reference {
    std::vector<std::uint64_t> bytes;
    int pointer = 0;
    std::uint64_t last_id = 0;

    std::vector<Region> regions() const
    {
        std::vector<Region> result;
        for (int start = 0; start < static_cast<int>(bytes.size());) {
            int end = start + 1;
            while (end < static_cast<int>(bytes.size()) && bytes[end] == bytes[start]) ++end;
            result.push_back({start, end - start, bytes[start]});
            start = end;
        }
        return result;
    }

    void init(int capacity)
    {
        bytes.assign(capacity, 0);
        pointer = 0;
        last_id = 0;
    }

    int alloc(int size, FitAlg algorithm)
    {
        if (size <= 0) return -1;
        auto all = regions();
        std::vector<Region> holes;
        for (const auto& region : all) if (region.id == 0) holes.push_back(region);
        if (algorithm == FitAlg::Next && !holes.empty()) {
            auto first = std::find_if(holes.begin(), holes.end(), [this](const Region& h) {
                return h.start + h.size > pointer;
            });
            if (first == holes.end()) first = holes.begin();
            std::rotate(holes.begin(), first, holes.end());
        }
        const Region* chosen = nullptr;
        for (const auto& hole : holes) {
            if (hole.size < size) continue;
            if (!chosen || (algorithm == FitAlg::Best && hole.size < chosen->size) ||
                (algorithm == FitAlg::Worst && hole.size > chosen->size)) chosen = &hole;
            if (algorithm == FitAlg::First || algorithm == FitAlg::Next) break;
        }
        if (!chosen) return -1;
        const auto id = ++last_id;
        for (int i = chosen->start; i < chosen->start + size; ++i) bytes[i] = id;
        pointer = chosen->start + size;
        return chosen->start;
    }

    bool freeId(std::uint64_t id)
    {
        if (id == 0 || std::find(bytes.begin(), bytes.end(), id) == bytes.end()) return false;
        for (auto& byte : bytes) if (byte == id) byte = 0;
        return true;
    }

    bool freeAddr(int address)
    {
        if (address < 0 || address >= static_cast<int>(bytes.size()) || bytes[address] == 0 ||
            (address > 0 && bytes[address - 1] == bytes[address])) return false;
        return freeId(bytes[address]);
    }

    int defrag(DefragOption option)
    {
        std::vector<Region> active;
        for (const auto& region : regions()) if (region.id != 0) active.push_back(region);
        const int capacity = static_cast<int>(bytes.size());
        int used = 0;
        for (const auto& region : active) used += region.size;
        int best_cost = capacity + 1;
        std::size_t best_cut = 0;
        for (std::size_t cut = 0; cut <= active.size(); ++cut) {
            if ((option == DefragOption::Init && cut != 0) ||
                (option == DefragOption::End && cut != active.size())) continue;
            int cost = 0;
            int left = 0;
            int right = capacity - used;
            for (std::size_t i = 0; i < cut; ++i) right += active[i].size;
            for (std::size_t i = 0; i < active.size(); ++i) {
                const int target = i < cut ? left : right;
                if (active[i].start != target) cost += active[i].size;
                if (i < cut) left += active[i].size;
                else right += active[i].size;
            }
            if (cost <= best_cost) { best_cost = cost; best_cut = cut; }
        }
        std::fill(bytes.begin(), bytes.end(), 0);
        int left = 0;
        int right = capacity - used;
        for (std::size_t i = 0; i < best_cut; ++i) right += active[i].size;
        for (std::size_t i = 0; i < active.size(); ++i) {
            const int target = i < best_cut ? left : right;
            for (int pos = target; pos < target + active[i].size; ++pos) bytes[pos] = active[i].id;
            if (i < best_cut) left += active[i].size;
            else right += active[i].size;
        }
        pointer = 0;
        return best_cost;
    }
};

void compare(const MemoryManager& actual, const Reference& expected, int seed, int step)
{
    check(actual.getCapacity() == static_cast<int>(expected.bytes.size()), seed, step, "capacidade");
    check(actual.getNextPointer() == expected.pointer, seed, step, "ponteiro");
    const auto expected_regions = expected.regions();
    check(actual.getBlocks().size() == expected_regions.size(), seed, step, "numero de blocos/buracos");
    std::vector<std::uint64_t> actual_bytes(expected.bytes.size());
    std::size_t region_index = 0;
    for (const auto& block : actual.getBlocks()) {
        check(block.start >= 0 && block.size > 0 && block.getEnd() <= actual.getCapacity(), seed, step, "intervalo");
        const auto& region = expected_regions[region_index++];
        check(block.start == region.start && block.size == region.size && block.id == region.id,
              seed, step, "lista de blocos/buracos");
        for (int pos = block.start; pos < block.getEnd(); ++pos) actual_bytes[pos] = block.id;
    }
    check(actual_bytes == expected.bytes, seed, step, "mapa de bytes e IDs");
    const auto stats = std::get<Stats>(actual.getStats());
    int free = 0, holes = 0, largest = 0;
    for (const auto& region : expected.regions()) if (region.id == 0) {
        free += region.size; ++holes; largest = std::max(largest, region.size);
    }
    check(stats.free == free && stats.occupied == actual.getCapacity() - free &&
          stats.hole_count == static_cast<std::size_t>(holes) && stats.largest_hole == largest,
          seed, step, "estatisticas");
}

void testCoalescence()
{
    const auto prepared = [] {
        MemoryManager memory;
        memory.init(16);
        for (int i = 0; i < 4; ++i) memory.alloc(4, FitAlg::First);
        return memory;
    };
    auto left = prepared();
    left.freeId(1);
    auto result = left.freeId(2);
    check(result.merged_from.size() == 1 && result.merged_into &&
          result.merged_into->start == 0 && result.merged_into->size == 8, 0, 0, "coalescencia esquerda");
    auto right = prepared();
    right.freeId(3);
    result = right.freeId(2);
    check(result.merged_from.size() == 1 && result.merged_into &&
          result.merged_into->start == 4 && result.merged_into->size == 8, 0, 0, "coalescencia direita");
    auto both = prepared();
    both.freeId(2);
    both.freeId(4);
    result = both.freeId(3);
    check(result.merged_from.size() == 2 && result.merged_into &&
          result.merged_into->start == 4 && result.merged_into->size == 12, 0, 0, "coalescencia em ambos os lados");
}
}

int main()
{
    testCoalescence();
    for (int seed = 1; seed <= 8; ++seed) {
        std::mt19937 random(seed);
        MemoryManager actual;
        Reference expected;
        const int initial = 1 + static_cast<int>(random() % 64);
        actual.init(initial);
        expected.init(initial);
        for (int step = 0; step < 2000; ++step) {
            const int action = static_cast<int>(random() % 100);
            if (action < 48) {
                const int size = static_cast<int>(random() % 18);
                const auto algorithm = static_cast<FitAlg>(random() % 4);
                const int start = expected.alloc(size, algorithm);
                const auto result = actual.alloc(size, algorithm);
                check(result.isOk() == (start >= 0), seed, step, "sucesso alloc");
                if (start >= 0) check(result.allocated && result.allocated->start == start &&
                    result.allocated->id == expected.last_id, seed, step, "endereco/ID alloc");
            } else if (action < 66) {
                const auto id = static_cast<std::uint64_t>(random() % (expected.last_id + 3));
                check(actual.freeId(id).isOk() == expected.freeId(id), seed, step, "freeid");
            } else if (action < 80) {
                const int address = static_cast<int>(random() % (expected.bytes.size() + 2));
                check(actual.freeAddr(address).isOk() == expected.freeAddr(address), seed, step, "freeaddr");
            } else if (action < 96) {
                const auto option = static_cast<DefragOption>(random() % 3);
                const int cost = expected.defrag(option);
                const auto result = actual.defrag(option);
                check(result.isOk() && result.moved_bytes == cost, seed, step, "defrag e custo");
            } else {
                const int capacity = 1 + static_cast<int>(random() % 64);
                check(actual.init(capacity).isOk(), seed, step, "init");
                expected.init(capacity);
            }
            compare(actual, expected, seed, step);
        }
    }
}

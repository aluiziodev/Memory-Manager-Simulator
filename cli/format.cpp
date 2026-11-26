#include "format.hpp"

#include <algorithm>
#include <array>
#include <iomanip>
#include <locale>
#include <sstream>

namespace memsim::cli {
namespace {

void repeat(std::ostream& output, char symbol, std::int64_t count)
{
    std::array<char, 1024> chunk;
    chunk.fill(symbol);
    while (count > 0 && output) {
        const auto length = std::min(count, static_cast<std::int64_t>(chunk.size()));
        output.write(chunk.data(), length);
        count -= length;
    }
}

void writeStats(std::ostream& output, const Stats& stats)
{
    std::ostringstream text;
    text.imbue(std::locale::classic());
    text << "== Estatísticas ==\n"
         << "Tamanho total: " << stats.capacity << " bytes\n"
         << "Ocupado: " << stats.occupied << " bytes | Livre: " << stats.free << " bytes\n"
         << "Buracos (fragmentação externa): " << stats.hole_count << '\n'
         << "Maior buraco livre: " << stats.largest_hole << " bytes\n"
         << "Uso efetivo: " << std::fixed << std::setprecision(2) << stats.usage_percent << "%\n";
    output << text.str();
}

void writeMapRow(std::ostream& output, const std::vector<Block>& blocks, bool show_ids)
{
    output << '[';
    for (const auto& block : blocks) {
        const char occupied = show_ids ? static_cast<char>('0' + block.id % 10) : '#';
        repeat(output, block.isFree() ? '.' : occupied, block.size);
    }
    output << "]\n";
}

void writeView(std::ostream& output, const script::MemoryView& view)
{
    const auto border_width = static_cast<std::int64_t>(view.stats.capacity) + 2;
    output << "Mapa de Memória (" << std::to_string(view.stats.capacity) << " bytes)\n";
    repeat(output, '-', border_width);
    output << '\n';
    writeMapRow(output, view.blocks, false);
    writeMapRow(output, view.blocks, true);
    repeat(output, '-', border_width);
    output << "\n\nBlocos ativos: ";
    const char* separator = "";
    for (const auto& block : view.blocks) {
        if (block.isFree()) {
            continue;
        }
        const auto size = std::to_string(block.size);
        output << separator << "[id=" << std::to_string(block.id) << "] @"
               << std::to_string(block.start) << " +" << size << "B (usado=" << size << "B)";
        separator = " | ";
    }
    if (view.stats.occupied == 0) {
        output << "(nenhum)";
    }
    output << '\n';
    writeStats(output, view.stats);
}

void writeError(std::ostream& output, const OperationError& error)
{
    output << "Erro: ";
    switch (error.code) {
    case ErrorCode::NotInitialized:
        output << "memória não inicializada";
        break;
    case ErrorCode::InvalidSize:
        output << "tamanho inválido";
        break;
    case ErrorCode::UnknownBlockId:
        output << "ID de bloco inexistente";
        break;
    case ErrorCode::InvalidBlockStart:
        output << "endereço não é o início de um bloco alocado";
        break;
    case ErrorCode::NoContiguousSpace:
        output << "sem espaço contíguo";
        if (const auto& shortage = error.space_shortage) {
            output << " para " << std::to_string(shortage->requested)
                   << " bytes (livre total: " << std::to_string(shortage->total_free)
                   << ", maior buraco: " << std::to_string(shortage->largest_hole) << ')';
        }
        break;
    case ErrorCode::IdExhausted:
        output << "limite de IDs de bloco atingido";
        break;
    case ErrorCode::UnsupportedAlgorithm:
        output << "algoritmo ou modo de compactação inválido";
        break;
    }
    output << '\n';
}

const char* defragName(DefragOption option)
{
    switch (option) {
    case DefragOption::Init: return "INIT";
    case DefragOption::End: return "END";
    case DefragOption::Opt: return "OPT";
    }
    return "";
}

} // namespace

void writeResult(std::ostream& output, const script::Command& command,
                 const script::CommandResult& result)
{
    if (const auto* view = std::get_if<script::MemoryView>(&result)) {
        writeView(output, *view);
    } else if (const auto* stats = std::get_if<Stats>(&result)) {
        writeStats(output, *stats);
    } else if (const auto* error = std::get_if<OperationError>(&result)) {
        writeError(output, *error);
    } else {
        const auto& step = std::get<StepResult>(result);
        if (step.error) {
            writeError(output, *step.error);
        } else if (const auto* defrag = std::get_if<script::Defrag>(&command.args)) {
            output << "Defrag " << defragName(defrag->option) << ": "
                   << std::to_string(step.moved.size())
                   << (step.moved.size() == 1 ? " bloco movido, " : " blocos movidos, ")
                   << std::to_string(step.moved_bytes) << " bytes deslocados\n";
        }
    }
}

} // namespace memsim::cli

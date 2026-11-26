#include "step_panel.hpp"

#include <QLabel>
#include <QVBoxLayout>

namespace memsim::visual {
namespace {

// Mostra um erro de operação (Caso haja)
QString errorText(const OperationError& error)
{
    switch (error.code) {
    case ErrorCode::NoContiguousSpace:
        if (error.space_shortage) {
            const auto& space = *error.space_shortage;
            return QStringLiteral("Erro: sem espaço contíguo para %1 bytes (livre total: %2, maior buraco: %3)")
                .arg(space.requested).arg(space.total_free).arg(space.largest_hole);
        }
        return QStringLiteral("Erro: sem espaço contíguo.");
    case ErrorCode::NotInitialized: return QStringLiteral("Erro: memória não inicializada.");
    case ErrorCode::InvalidSize: return QStringLiteral("Erro: tamanho inválido.");
    case ErrorCode::UnknownBlockId: return QStringLiteral("Erro: ID inexistente.");
    case ErrorCode::InvalidBlockStart: return QStringLiteral("Erro: endereço não é início de bloco alocado.");
    case ErrorCode::IdExhausted: return QStringLiteral("Erro: IDs esgotados.");
    case ErrorCode::UnsupportedAlgorithm: return QStringLiteral("Erro: algoritmo inválido.");
    }
    return QStringLiteral("Erro na operação.");
}

QString blockText(const Block& block)
{
    return QStringLiteral("@%1 +%2 B").arg(block.start).arg(block.size);
}

// traz infos do resultado de um passo
QString describe(const StepResult& result)
{
    QStringList details;
    if (result.chosen) details << QStringLiteral("Buraco escolhido antes da divisão: %1").arg(blockText(*result.chosen));
    if (!result.examined.empty()) {
        QStringList examined;
        for (const Block& block : result.examined) examined << blockText(block);
        details << QStringLiteral("Buracos examinados: %1").arg(examined.join(QStringLiteral(" → ")));
    }
    if (result.allocated) details << QStringLiteral("Alocado: ID %1 em %2")
        .arg(static_cast<qulonglong>(result.allocated->id)).arg(blockText(*result.allocated));
    if (result.freed) details << QStringLiteral("Liberado: ID %1 em %2")
        .arg(static_cast<qulonglong>(result.freed->id)).arg(blockText(*result.freed));
    if (result.merged_into) details << QStringLiteral("Coalescência: %1 buraco(s) unido(s) em %2")
        .arg(static_cast<qulonglong>(result.merged_from.size())).arg(blockText(*result.merged_into));
    if (!result.moved.empty()) {
        QStringList moves;
        for (const auto& move : result.moved) moves << QStringLiteral("ID %1: @%2 → @%3 (%4 B)")
            .arg(static_cast<qulonglong>(move.id)).arg(move.from).arg(move.to).arg(move.size);
        details << QStringLiteral("Movimentos: %1").arg(moves.join(QStringLiteral("; ")));
    }
    if (result.moved_bytes || !result.moved.empty()) details << QStringLiteral("Total deslocado: %1 B").arg(result.moved_bytes);
    details << QStringLiteral("Ponteiro: busca em @%1; após o passo @%2")
        .arg(result.next_pointer_before).arg(result.next_pointer_after);
    return details.join(QLatin1Char('\n'));
}
}

// Painel de informações do passo atual, tendo o comando, resultado e as estatísticas.
StepPanel::StepPanel(QWidget* parent) : QGroupBox(QStringLiteral("Passo atual"), parent)
{
    auto* layout = new QVBoxLayout(this);
    command_ = new QLabel(this);
    error_ = new QLabel(this);
    error_->setObjectName(QStringLiteral("operationErrorBanner"));
    error_->setWordWrap(true);
    error_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    error_->setStyleSheet(QStringLiteral(
        "QLabel { background: #fff0ed; color: #8b1e1e; border: 2px solid #c43c32; "
        "border-radius: 6px; padding: 10px 12px; font-weight: 700; }"));
    error_->hide();
    stats_ = new QLabel(this);
    details_ = new QLabel(this);
    details_->setWordWrap(true);
    layout->addWidget(command_);
    layout->addWidget(error_);
    layout->addWidget(stats_);
    layout->addWidget(details_);
}

// Atualiza o painel
void StepPanel::setSession(const Session& session)
{
    const auto* result = session.currentResult();
    const auto* step = result ? std::get_if<StepResult>(result) : nullptr;
    const bool failed = step && step->error.has_value();
    setTitle(failed ? QStringLiteral("Erro no passo %1").arg(static_cast<qulonglong>(session.step()))
                    : QStringLiteral("Passo atual"));


    error_->setText(failed
        ? QStringLiteral("OPERAÇÃO NÃO REALIZADA\n%1").arg(errorText(*step->error)) : QString());

    error_->setVisible(failed);
    command_->setStyleSheet(failed ? QStringLiteral("color: #8b1e1e; font-weight: 700;") : QString());
    command_->setText(session.currentCommand()
        ? QStringLiteral("Comando: %1 (linha %2)").arg(QString::fromStdString(session.currentCommand()->source.text).trimmed()).arg(static_cast<qulonglong>(session.currentCommand()->source.number))
        : QStringLiteral("Aguardando o primeiro comando"));
    const auto stats = session.memory().getStats();
    if (const auto* value = std::get_if<Stats>(&stats)) {
        stats_->setText(QStringLiteral("Ocupado: %1 B  |  Livre: %2 B  |  Buracos: %3  |  Maior: %4 B  |  Uso: %5%")
            .arg(value->occupied).arg(value->free)
            .arg(static_cast<qulonglong>(value->hole_count)).arg(value->largest_hole)
            .arg(value->usage_percent, 0, 'f', 2));
    } else {
        stats_->setText(QStringLiteral("Memória não inicializada"));
    }
    QString details = result && std::holds_alternative<StepResult>(*result)
        ? describe(std::get<StepResult>(*result)) : QString();
    if (session.currentCommand() && std::holds_alternative<script::Defrag>(session.currentCommand()->args) &&
        result && std::holds_alternative<StepResult>(*result) &&
        std::get<StepResult>(*result).moved.empty()) {
        details.prepend(QStringLiteral("Compactação: 0 blocos movidos, 0 B deslocados\n"));
    }
    details_->setText(details);
}

} // namespace memsim::visual

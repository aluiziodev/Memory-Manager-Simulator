#include "memory_map.hpp"

#include <QEvent>
#include <QHelpEvent>
#include <QPainter>
#include <QToolTip>

#include <algorithm>

namespace memsim::visual {
namespace {
constexpr int margin = 24;
constexpr int top = 20;
constexpr int barHeight = 66;
}

MemoryMap::MemoryMap(QWidget* parent) : QWidget(parent)
{
    setMinimumHeight(132);
    setMinimumWidth(320);
}

void MemoryMap::setMemory(const MemoryManager& memory)
{
    capacity_ = memory.getCapacity();
    blocks_ = memory.getBlocks();
    pointer_ = memory.getNextPointer();
    update();
}

void MemoryMap::setStepResult(const StepResult* result)
{
    result_ = result ? std::optional<StepResult>(*result) : std::nullopt;
    update();
}

QColor MemoryMap::colorForId(std::uint64_t id)
{
    if (id == 0) return QColor(220, 225, 230);
    // A cor do bloco depende do ID 
    return QColor::fromHsv(static_cast<int>((id * 137u) % 360u), 125, 205);
}

const Block* MemoryMap::blockAt(int x) const noexcept
{
    const int usable = width() - 2 * margin;
    if (capacity_ <= 0 || usable <= 0 || x < margin || x >= margin + usable) return nullptr;
    const auto address = static_cast<int>((static_cast<std::int64_t>(x - margin) * capacity_) / usable);
    const auto it = std::find_if(blocks_.begin(), blocks_.end(), [address](const Block& block) {
        return block.start <= address && address < block.getEnd();
    });
    return it == blocks_.end() ? nullptr : &*it;
}

void MemoryMap::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);
    const int usable = width() - 2 * margin;
    if (capacity_ <= 0 || usable <= 0) {
        painter.drawText(rect(), Qt::AlignCenter, QStringLiteral("Memória não inicializada"));
        return;
    }
    painter.fillRect(QRect(margin, top, usable, barHeight), palette().base());
    for (const Block& block : blocks_) {
        const qreal left = margin + static_cast<qreal>(block.start) * usable / capacity_;
        const qreal right = margin + static_cast<qreal>(block.getEnd()) * usable / capacity_;
        const QRectF region(left, top, right - left, barHeight);
        
        painter.setPen(Qt::NoPen);
        painter.setBrush(block.isFree() ? QBrush(colorForId(0), Qt::Dense4Pattern): QBrush(colorForId(block.id)));

        painter.drawRect(region);
        painter.setPen(QColor(48, 54, 60));
        painter.drawLine(QPointF(left, top), QPointF(left, top + barHeight));
        if (region.width() >= 75) {
            const QString label = block.isFree()
                ? QStringLiteral("Livre\n@%1 +%2 B").arg(block.start).arg(block.size)
                : QStringLiteral("ID %1\n@%2 +%3 B").arg(static_cast<qulonglong>(block.id)).arg(block.start).arg(block.size);

            painter.drawText(region.adjusted(2, 2, -2, -2), Qt::AlignCenter, label);
        }
    }
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QColor(48, 54, 60));
    painter.drawRect(QRect(margin, top, usable, barHeight));
    
    // Desenhar contornos de blocos especiais (escolhido, alocado, liberado,movido).
    const auto outline = [&](const Block& block, const QColor& color, int inset) {
        const qreal left = margin + static_cast<qreal>(block.start) * usable / capacity_;
        const qreal right = margin + static_cast<qreal>(block.getEnd()) * usable / capacity_;
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(color, 3, Qt::DashLine));
        painter.drawRect(QRectF(left + inset, top + inset, std::max<qreal>(1, right - left - 2 * inset), barHeight - 2 * inset));
    };


    if (result_) {
        if (result_->chosen) outline(*result_->chosen, QColor(210, 125, 0), 2);
        if (result_->allocated) outline(*result_->allocated, QColor(0, 105, 180), 6);
        if (result_->freed) outline(*result_->freed, QColor(195, 45, 70), 6);
        if (result_->merged_into) outline(*result_->merged_into, QColor(130, 60, 170), 2);
        for (const MovedBlock& moved : result_->moved) {
            outline(Block{moved.to, moved.size, moved.id}, QColor(10, 130, 90), 5);
        }
    }

    const int pointerX = margin + static_cast<int>(static_cast<qreal>(pointer_) * usable / capacity_);
    painter.setPen(QPen(QColor(155, 30, 125), 2));
    painter.drawLine(pointerX, top - 10, pointerX, top + barHeight + 4);

    const int y = top + barHeight + 8;

    // Desenhar marcações de endereço.
    for (int i = 0; i <= 4; ++i) {
        const int x = margin + usable * i / 4;
        painter.drawLine(x, y - 4, x, y + 2);
        const QString value = QString::number(static_cast<qint64>(capacity_) * i / 4);
        const QRect label(x - 35, y + 3, 70, 20);
        painter.drawText(label, Qt::AlignHCenter, value);
    }
}

bool MemoryMap::event(QEvent* event)
{
    if (event->type() == QEvent::ToolTip) {
        auto* help = static_cast<QHelpEvent*>(event);
        const Block* block = blockAt(help->pos().x());
        if (block && help->pos().y() >= top && help->pos().y() < top + barHeight) {
            const QString text = block->isFree()
                ? QStringLiteral("Livre · @%1 +%2 B").arg(block->start).arg(block->size)
                : QStringLiteral("ID %1 · @%2 +%3 B")
                    .arg(static_cast<qulonglong>(block->id)).arg(block->start).arg(block->size);
            QToolTip::showText(help->globalPos(), text, this);
            return true;
        }
        QToolTip::hideText();
        return true;
    }
    return QWidget::event(event);
}

} // namespace memsim::visual

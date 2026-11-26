#ifndef MEMSIM_VISUAL_QT_MEMORY_MAP_HPP
#define MEMSIM_VISUAL_QT_MEMORY_MAP_HPP

#include "core/memory_manager.hpp"

#include <QColor>
#include <QWidget>

namespace memsim::visual {

class MemoryMap : public QWidget {
public:
    explicit MemoryMap(QWidget* parent = nullptr);
    void setMemory(const MemoryManager& memory);
    void setStepResult(const StepResult* result);
    static QColor colorForId(std::uint64_t id);
    const Block* blockAt(int x) const noexcept;

protected:
    void paintEvent(QPaintEvent*) override;
    bool event(QEvent* event) override;

private:
    int capacity_ = 0;
    std::vector<Block> blocks_;
    int pointer_ = 0;
    std::optional<StepResult> result_;
};

} // namespace memsim::visual

#endif

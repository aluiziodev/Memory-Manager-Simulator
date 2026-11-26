#ifndef MEMSIM_VISUAL_QT_STEP_PANEL_HPP
#define MEMSIM_VISUAL_QT_STEP_PANEL_HPP

#include "visual/session.hpp"

#include <QGroupBox>

class QLabel;

namespace memsim::visual {

class StepPanel : public QGroupBox {
public:
    explicit StepPanel(QWidget* parent = nullptr);
    void setSession(const Session& session);

private:
    QLabel* command_;
    QLabel* error_;
    QLabel* stats_;
    QLabel* details_;
};

} // namespace memsim::visual

#endif

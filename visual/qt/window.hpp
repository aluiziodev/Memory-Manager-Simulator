#ifndef MEMSIM_VISUAL_QT_WINDOW_HPP
#define MEMSIM_VISUAL_QT_WINDOW_HPP

#include "visual/session.hpp"

#include <QMainWindow>

class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QTimer;
class QTableWidget;

namespace memsim::visual {

class MemoryMap;
class StepPanel;

class Window : public QMainWindow {
public:
    Window();
    bool runSmokeTest(const QString& path);
    bool runPlaybackTest(const QString& path);

private:
    QString loadScript(const QString& path);
    void refresh();
    void pause();
    void play();

    Session session_;
    QLabel* status_;
    QPlainTextEdit* preview_;
    MemoryMap* map_;
    StepPanel* panel_;
    QTableWidget* blocks_;
    QPushButton* advance_;
    QPushButton* back_;
    QPushButton* restart_;
    QPushButton* play_;
    QPushButton* pause_;
    QLineEdit* interval_;
    QTimer* timer_;
};

} // namespace memsim::visual

#endif

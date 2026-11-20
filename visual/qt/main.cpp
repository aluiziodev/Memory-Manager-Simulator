#include "core/block.hpp"

#include <QApplication>
#include <QFile>
#include <QFileDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <array>
#include <cstdlib>

namespace {

using Frame = std::array<memsim::Block, 3>;
constexpr int capacity = 32;
constexpr int intervalMs = 500;

// Estados fixos para validar o desenho e o temporizador antes de integrar o núcleo.
constexpr std::array<Frame, 3> frames{{
    {{{0, 8, 1}, {8, 8, 0}, {16, 16, 2}}},
    {{{0, 8, 1}, {8, 4, 3}, {12, 20, 0}}},
    {{{0, 12, 0}, {12, 4, 3}, {16, 16, 2}}},
}};

class MemoryMap : public QWidget {
public:
    MemoryMap()
    {
        setMinimumSize(440, 100);
    }

    void setFrame(std::size_t frame)
    {
        frame_ = frame;
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        constexpr int margin = 12;
        constexpr int top = 10;
        constexpr int height = 54;
        const double pixelsPerByte = (width() - 2 * margin) / double(capacity);

        for (const auto& block : frames[frame_]) {
            const QRectF rect(margin + block.start * pixelsPerByte, top,
                              block.size * pixelsPerByte, height);
            const QColor color = block.is_free()
                ? QColor(Qt::lightGray)
                : QColor::fromHsv(static_cast<int>(block.id % 360) * 137 % 360, 140, 225);
            painter.setBrush(block.is_free() ? QBrush(color, Qt::Dense4Pattern)
                                           : QBrush(color));
            painter.setPen(Qt::black);
            painter.drawRect(rect);
            const QString label = block.is_free()
                ? QStringLiteral("Livre\n%1 B").arg(block.size)
                : QStringLiteral("ID %1\n%2 B").arg(static_cast<qulonglong>(block.id)).arg(block.size);
            painter.drawText(rect, Qt::AlignCenter, label);
        }

        painter.setPen(palette().color(QPalette::WindowText));
        const QRect scale(margin, top + height + 6, width() - 2 * margin, 20);
        painter.drawText(scale, Qt::AlignLeft, QStringLiteral("0"));
        painter.drawText(scale, Qt::AlignRight, QString::number(capacity));
    }

private:
    std::size_t frame_ = 0;
};

class Prototype : public QWidget {
public:
    Prototype()
    {
        setWindowTitle(QStringLiteral("Simulador de memória — Protótipo F01"));
        resize(560, 380);

        auto* layout = new QVBoxLayout(this);
        auto* open = new QPushButton(QStringLiteral("Abrir roteiro…"), this);
        auto* play = new QPushButton(QStringLiteral("Reproduzir exemplo"), this);
        auto* pause = new QPushButton(QStringLiteral("Pausar"), this);

        preview_.setReadOnly(true);
        preview_.setPlaceholderText(QStringLiteral("Conteúdo do roteiro selecionado"));

        layout->addWidget(open);
        layout->addWidget(&preview_);
        layout->addWidget(&status_);
        layout->addWidget(&map_);
        layout->addWidget(play);
        layout->addWidget(pause);

        timer_.setInterval(intervalMs);
        connect(&timer_, &QTimer::timeout, this, [this] {
            showFrame(frame_ + 1);
            if (frame_ == frames.size() - 1) {
                timer_.stop();
                if (smokeTest_) {
                    QApplication::exit(EXIT_SUCCESS);
                }
            }
        });
        connect(play, &QPushButton::clicked, this, [this] { playExample(); });
        connect(pause, &QPushButton::clicked, &timer_, &QTimer::stop);
        connect(open, &QPushButton::clicked, this, [this] {
            timer_.stop();
            const QString path = QFileDialog::getOpenFileName(
                this, QStringLiteral("Abrir roteiro"), {},
                QStringLiteral("Roteiros (*.cmd);;Todos os arquivos (*)"));
            if (!path.isEmpty() && !loadScript(path)) {
                QMessageBox::warning(this, QStringLiteral("Erro ao abrir roteiro"),
                                     QStringLiteral("Não foi possível ler o arquivo."));
            }
        });

        showFrame(0);
    }

    bool runSmokeTest(const QString& path)
    {
        if (!loadScript(path) ||
            preview_.toPlainText().split(QLatin1Char('\n')).value(0) != QStringLiteral("init 32")) {
            return false;
        }
        smokeTest_ = true;
        playExample();
        return true;
    }

private:
    bool loadScript(const QString& path)
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return false;
        }
        const QByteArray contents = file.readAll();
        if (file.error() != QFileDevice::NoError) {
            return false;
        }
        preview_.setPlainText(QString::fromUtf8(contents));
        return true;
    }

    void playExample()
    {
        showFrame(0);
        timer_.start();
    }

    void showFrame(std::size_t frame)
    {
        frame_ = frame;
        map_.setFrame(frame_);
        status_.setText(QStringLiteral("Demonstração fixa · Estado %1 de %2 · Intervalo de 500 ms")
                            .arg(frame_ + 1)
                            .arg(frames.size()));
    }

    QPlainTextEdit preview_;
    QLabel status_;
    MemoryMap map_;
    QTimer timer_;
    std::size_t frame_ = 0;
    bool smokeTest_ = false;
};

} // namespace

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    Prototype window;
    window.show();

    const auto args = app.arguments();
    if (args.size() == 3 && args[1] == QStringLiteral("--smoke-test")) {
        if (!window.runSmokeTest(args[2])) {
            return EXIT_FAILURE;
        }
    }
    return app.exec();
}

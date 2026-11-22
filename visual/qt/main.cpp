#include "script/command.hpp"

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
#include <sstream>

namespace {

using Frame = std::array<memsim::Block, 3>;
constexpr int capacity = 32;
constexpr int interval_ms = 500;

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
        current_frame = frame;
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        constexpr int margin = 12;
        constexpr int top = 10;
        constexpr int height = 54;
        const double pixels_per_byte = (width() - 2 * margin) / double(capacity);

        for (const auto& block : frames[current_frame]) {
            const QRectF rect(margin + block.start * pixels_per_byte, top,
                              block.size * pixels_per_byte, height);
            const QColor color = block.isFree()
                ? QColor(Qt::lightGray)
                : QColor::fromHsv(static_cast<int>(block.id % 360) * 137 % 360, 140, 225);
            painter.setBrush(block.isFree() ? QBrush(color, Qt::Dense4Pattern)
                                           : QBrush(color));
            painter.setPen(Qt::black);
            painter.drawRect(rect);
            const QString label = block.isFree()
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
    std::size_t current_frame = 0;
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

        preview.setReadOnly(true);
        preview.setPlaceholderText(QStringLiteral("Conteúdo do roteiro selecionado"));

        layout->addWidget(open);
        layout->addWidget(&preview);
        layout->addWidget(&status);
        layout->addWidget(&map);
        layout->addWidget(play);
        layout->addWidget(pause);

        timer.setInterval(interval_ms);
        connect(&timer, &QTimer::timeout, this, [this] {
            showFrame(current_frame + 1);
            if (current_frame == frames.size() - 1) {
                timer.stop();
                if (smoke_test) {
                    QApplication::exit(EXIT_SUCCESS);
                }
            }
        });
        connect(play, &QPushButton::clicked, this, [this] { playExample(); });
        connect(pause, &QPushButton::clicked, &timer, &QTimer::stop);
        connect(open, &QPushButton::clicked, this, [this] {
            const QString path = QFileDialog::getOpenFileName(
                this, QStringLiteral("Abrir roteiro"), {},
                QStringLiteral("Roteiros (*.cmd);;Todos os arquivos (*)"));
            if (path.isEmpty()) {
                return;
            }
            const QString error = loadScript(path);
            if (!error.isEmpty()) {
                QMessageBox::warning(this, QStringLiteral("Erro ao abrir roteiro"), error);
            }
        });

        showFrame(0);
    }

    bool runSmokeTest(const QString& path)
    {
        if (!loadScript(path).isEmpty() ||
            preview.toPlainText().split(QLatin1Char('\n')).value(0) != QStringLiteral("init 32")) {
            return false;
        }
        smoke_test = true;
        playExample();
        return true;
    }

private:
    QString loadScript(const QString& path)
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            return QStringLiteral("Não foi possível abrir o arquivo.");
        }
        const QByteArray contents = file.readAll();
        if (file.error() != QFileDevice::NoError) {
            return QStringLiteral("Não foi possível ler o arquivo.");
        }
        std::istringstream input(contents.toStdString());
        const auto parsed = memsim::script::readVisualScript(input);
        if (const auto* error = std::get_if<memsim::script::ParseError>(&parsed)) {
            const QString message = QString::fromStdString(error->message);
            if (error->source.number != 0) {
                return QStringLiteral("Linha %1 — %2")
                    .arg(static_cast<qulonglong>(error->source.number)).arg(message);
            }
            return message;
        }
        timer.stop();
        preview.setPlainText(QString::fromUtf8(contents));
        return {};
    }

    void playExample()
    {
        showFrame(0);
        timer.start();
    }

    void showFrame(std::size_t frame)
    {
        current_frame = frame;
        map.setFrame(current_frame);
        status.setText(QStringLiteral("Demonstração fixa · Estado %1 de %2 · Intervalo de 500 ms")
                            .arg(current_frame + 1)
                            .arg(frames.size()));
    }

    QPlainTextEdit preview;
    QLabel status;
    MemoryMap map;
    QTimer timer;
    std::size_t current_frame = 0;
    bool smoke_test = false;
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

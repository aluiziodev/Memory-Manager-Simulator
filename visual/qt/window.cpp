#include "window.hpp"
#include "memory_map.hpp"
#include "playback_interval.hpp"
#include "step_panel.hpp"

#include <QFile>
#include <QFileDialog>
#include <QEventLoop>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <sstream>

namespace memsim::visual {

//Cria a janela principal do simulador
Window::Window()
{
    setWindowTitle(QStringLiteral("Simulador de gerência de memória"));
    resize(900, 720);
    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* layout = new QVBoxLayout(central);
    auto* open = new QPushButton(QStringLiteral("Abrir roteiro…"), central);
    preview_ = new QPlainTextEdit(central);
    preview_->setReadOnly(true);
    preview_->setMaximumHeight(110);
    preview_->setPlaceholderText(QStringLiteral("Selecione um arquivo .cmd"));
    status_ = new QLabel(central);
    map_ = new MemoryMap(central);
    panel_ = new StepPanel(central);
    blocks_ = new QTableWidget(central);
    blocks_->setColumnCount(4);
    blocks_->setHorizontalHeaderLabels({QStringLiteral("ID"), QStringLiteral("Início"),
                                       QStringLiteral("Tamanho"), QStringLiteral("Estado")});
    blocks_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    blocks_->setMinimumHeight(145);
    blocks_->horizontalHeader()->setStretchLastSection(true);
    advance_ = new QPushButton(QStringLiteral("Avançar"), central);
    back_ = new QPushButton(QStringLiteral("Voltar"), central);
    restart_ = new QPushButton(QStringLiteral("Reiniciar"), central);
    play_ = new QPushButton(QStringLiteral("Reproduzir"), central);
    pause_ = new QPushButton(QStringLiteral("Pausar"), central);
    interval_ = new QLineEdit(QStringLiteral("0,5"), central);
    interval_->setPlaceholderText(QStringLiteral("Segundos"));
    interval_->setMaximumWidth(95);
    timer_ = new QTimer(this);
    auto* controls = new QHBoxLayout;
    controls->addWidget(back_);
    controls->addWidget(advance_);
    controls->addWidget(restart_);
    controls->addWidget(new QLabel(QStringLiteral("Intervalo (s):"), central));
    controls->addWidget(interval_);
    controls->addWidget(play_);
    controls->addWidget(pause_);
    layout->addWidget(open);
    layout->addWidget(preview_);
    layout->addWidget(status_);
    layout->addWidget(map_);
    layout->addWidget(panel_);
    layout->addWidget(blocks_);
    layout->addLayout(controls);

    connect(open, &QPushButton::clicked, this, [this] {
        const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("Abrir roteiro"), {},
                                                         QStringLiteral("Roteiros (*.cmd);;Todos os arquivos (*)"));
        if (path.isEmpty()) return;
        const QString error = loadScript(path);
        if (!error.isEmpty()) QMessageBox::warning(this, QStringLiteral("Erro ao abrir roteiro"), error);
    });
    connect(advance_, &QPushButton::clicked, this, [this] { pause(); session_.advance(); refresh(); });
    connect(back_, &QPushButton::clicked, this, [this] { pause(); session_.back(); refresh(); });
    connect(restart_, &QPushButton::clicked, this, [this] { pause(); session_.restart(); refresh(); });
    connect(play_, &QPushButton::clicked, this, [this] { play(); });
    connect(pause_, &QPushButton::clicked, this, [this] { pause(); });
    connect(timer_, &QTimer::timeout, this, [this] {
        session_.advance();
        if (!session_.canAdvance()) pause();
        refresh();
    });
    refresh();
}

//Carrega um roteiro
QString Window::loadScript(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return QStringLiteral("Não foi possível abrir o arquivo.");
    const QByteArray contents = file.readAll();
    if (file.error() != QFileDevice::NoError) return QStringLiteral("Não foi possível ler o arquivo.");
    std::istringstream input(contents.toStdString());
    const auto loaded = session_.load(input);
    if (const auto* error = std::get_if<script::ParseError>(&loaded)) {
        return error->source.number == 0
            ? QString::fromStdString(error->message)
            : QStringLiteral("Linha %1 — %2").arg(static_cast<qulonglong>(error->source.number))
                  .arg(QString::fromStdString(error->message));
    }
    pause();
    preview_->setPlainText(QString::fromUtf8(contents));
    refresh();
    return {};
}

// Atualiza a interface.
void Window::refresh()
{
    map_->setMemory(session_.memory());
    const auto* result = session_.currentResult();
    map_->setStepResult(result ? std::get_if<StepResult>(result) : nullptr);
    panel_->setSession(session_);
    const bool failed = result && std::holds_alternative<StepResult>(*result) &&
                        std::get<StepResult>(*result).error.has_value();
    const auto& regions = session_.memory().getBlocks();
    blocks_->setRowCount(static_cast<int>(regions.size()));
    for (int row = 0; row < static_cast<int>(regions.size()); ++row) {
        const Block& block = regions[static_cast<std::size_t>(row)];
        const QStringList values = {
            block.isFree() ? QStringLiteral("—") : QString::number(static_cast<qulonglong>(block.id)),
            QString::number(block.start), QString::number(block.size),
            block.isFree() ? QStringLiteral("Livre") : QStringLiteral("Alocado")
        };
        for (int col = 0; col < values.size(); ++col) {
            auto* item = new QTableWidgetItem(values[col]);
            item->setBackground(MemoryMap::colorForId(block.id));
            blocks_->setItem(row, col, item);
        }
    }
    status_->setText(session_.hasScript()
        ? QStringLiteral("Passo %1 de %2%3")
              .arg(static_cast<qulonglong>(session_.step()))
              .arg(static_cast<qulonglong>(session_.total()))
              .arg(failed ? QStringLiteral(" · Erro neste passo")
                          : session_.canAdvance() ? QString() : QStringLiteral(" · Fim do roteiro"))
        : QStringLiteral("Nenhum roteiro carregado"));
    status_->setStyleSheet(failed ? QStringLiteral("color: #8b1e1e; font-weight: 700;") : QString());
    advance_->setEnabled(session_.canAdvance());
    back_->setEnabled(session_.canBack());
    restart_->setEnabled(session_.canBack());
    play_->setEnabled(session_.canAdvance());
    pause_->setEnabled(timer_->isActive());
}

void Window::pause()
{
    timer_->stop();
    pause_->setEnabled(false);
}

void Window::play()
{
    const auto milliseconds = parseInterval(interval_->text());
    if (!milliseconds) {
        QMessageBox::warning(this, QStringLiteral("Intervalo inválido"),
                             QStringLiteral("Informe segundos positivos, como 0,5 ou 1.25."));
        return;
    }
    if (!session_.canAdvance()) return;
    timer_->start(*milliseconds); // Um único QTimer; reiniciar não acumula conexões.
    refresh();
}

bool Window::runSmokeTest(const QString& path)
{
    if (!loadScript(path).isEmpty() || !session_.canAdvance()) return false;
    while (session_.canAdvance()) session_.advance();
    if (session_.step() != session_.total()) return false;
    while (session_.canBack()) session_.back();
    refresh();
    return session_.step() == 0 && !session_.memory().isInitialized();
}


bool Window::runPlaybackTest(const QString& path)
{
    if (!loadScript(path).isEmpty()) return false;
    interval_->setText(QStringLiteral("0,05"));
    play();
    play();
    QEventLoop loop;
    QTimer::singleShot(75, &loop, &QEventLoop::quit);
    loop.exec();
    if (session_.step() != 1) return false;
    pause();
    QTimer::singleShot(100, &loop, &QEventLoop::quit);
    loop.exec();
    return session_.step() == 1 && !timer_->isActive();
}


} // namespace memsim::visual

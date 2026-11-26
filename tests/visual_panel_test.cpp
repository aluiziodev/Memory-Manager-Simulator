#include "visual/qt/step_panel.hpp"

#include <QApplication>
#include <QLabel>

#include <cstdlib>
#include <fstream>
#include <sstream>

using namespace memsim::visual;

namespace {
bool contains(const StepPanel& panel, const QString& text)
{
    for (const QLabel* label : panel.findChildren<QLabel*>())
        if (label->text().contains(text)) return true;
    return false;
}
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    Session session;
    StepPanel panel;
    std::istringstream input("init 16\nalloc 8 next\nalloc 12 next\nfreeid 1\ndefrag end\n");
    if (!std::holds_alternative<std::monostate>(session.load(input))) return EXIT_FAILURE;
    session.advance(); session.advance();
    panel.setSession(session);
    if (!contains(panel, QStringLiteral("Buraco escolhido")) ||
        !contains(panel, QStringLiteral("busca em @0; após o passo @8")) ||
        !contains(panel, QStringLiteral("Ocupado: 8 B"))) return EXIT_FAILURE;
    session.advance(); panel.setSession(session);
    const auto* banner = panel.findChild<QLabel*>(QStringLiteral("operationErrorBanner"));
    if (!banner || banner->isHidden() ||
        !banner->text().contains(QStringLiteral("OPERAÇÃO NÃO REALIZADA")) ||
        !banner->text().contains(QStringLiteral("sem espaço contíguo para 12 bytes")) ||
        !panel.title().contains(QStringLiteral("Erro no passo 3"))) return EXIT_FAILURE;
    session.advance(); panel.setSession(session);
    if (!banner->isHidden() || panel.title() != QStringLiteral("Passo atual")) return EXIT_FAILURE;
    if (!contains(panel, QStringLiteral("Liberado")) ||
        !contains(panel, QStringLiteral("Coalescência"))) return EXIT_FAILURE;
    session.advance(); panel.setSession(session);
    if (!contains(panel, QStringLiteral("Ponteiro: busca em @8; após o passo @0"))) return EXIT_FAILURE;
    session.back(); panel.setSession(session);
    if (!contains(panel, QStringLiteral("Coalescência"))) return EXIT_FAILURE;
    if (argc != 2) return EXIT_FAILURE;
    std::ifstream example(argv[1]);
    Session fragmentation;
    if (!example || !std::holds_alternative<std::monostate>(fragmentation.load(example))) return EXIT_FAILURE;
    while (fragmentation.advance()) {}
    panel.setSession(fragmentation);
    if (banner->isHidden() ||
        !banner->text().contains(QStringLiteral("9 bytes (livre total: 16, maior buraco: 8)")))
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
}

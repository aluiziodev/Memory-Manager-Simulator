#include "playback_interval.hpp"

#include <QRegularExpression>

#include <cmath>
#include <limits>

namespace memsim::visual {

// Convertea entrada de segundos em uma reprodução em milissegundos.
std::optional<int> parseInterval(const QString& seconds)
{
    static const QRegularExpression syntax(QStringLiteral("^[0-9]+(?:[,.][0-9]+)?$"));
    const QString trimmed = seconds.trimmed();

    if (!syntax.match(trimmed).hasMatch()) return std::nullopt;
    QString normalized = trimmed;
    normalized.replace(QLatin1Char(','), QLatin1Char('.'));
    bool ok = false;

    const double milliseconds = normalized.toDouble(&ok) * 1000.0;
    if (!ok || !std::isfinite(milliseconds) || milliseconds < 1.0 ||
        milliseconds > std::numeric_limits<int>::max()) return std::nullopt;
        
    return static_cast<int>(std::llround(milliseconds));
}

} // namespace memsim::visual

#include "visual/qt/playback_interval.hpp"

#include <cstdlib>

using memsim::visual::parseInterval;

int main()
{
    if (parseInterval(QStringLiteral("0,5")) != 500 ||
        parseInterval(QStringLiteral("0.5")) != 500 ||
        parseInterval(QStringLiteral(" 1.25 ")) != 1250 ||
        parseInterval(QStringLiteral("0,001")) != 1) return EXIT_FAILURE;
    for (const QString& invalid : {QStringLiteral(""), QStringLiteral("0"), QStringLiteral("0,0005"),
                                   QStringLiteral("0,"), QStringLiteral("-1"), QStringLiteral("nan"),
                                   QStringLiteral("inf"), QStringLiteral("2147484"), QStringLiteral("1e3")}) {
        if (parseInterval(invalid)) return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

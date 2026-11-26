#ifndef MEMSIM_VISUAL_QT_PLAYBACK_INTERVAL_HPP
#define MEMSIM_VISUAL_QT_PLAYBACK_INTERVAL_HPP

#include <QString>

#include <optional>

namespace memsim::visual {

// Aceita ponto ou até vírgula; devolve milissegundos inteiros entre 1 e INT_MAX.
std::optional<int> parseInterval(const QString& seconds);

} 

#endif

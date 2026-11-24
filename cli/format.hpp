#ifndef MEMSIM_CLI_FORMAT_HPP
#define MEMSIM_CLI_FORMAT_HPP

#include "script/command.hpp"

#include <iosfwd>

namespace memsim::cli {

void writeResult(std::ostream& output, const script::Command& command,
                 const script::CommandResult& result);

} // namespace memsim::cli

#endif

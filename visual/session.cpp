#include "session.hpp"

#include <utility>

namespace memsim::visual {

Session::LoadResult Session::load(std::istream& input)
{
    auto parsed = script::readVisualScript(input);
    if (auto* error = std::get_if<script::ParseError>(&parsed)) {
        return *error; // Um arquivo inválido não substitui a sessão atual.
    }
    commands_ = std::move(std::get<std::vector<script::Command>>(parsed));
    history_.clear();
    history_.push_back({MemoryManager{}, std::nullopt});
    return std::monostate{};
}

bool Session::advance()
{
    if (!canAdvance()) return false;
    MemoryManager next = memory();
    auto result = script::execute(next, commands_[step()]);
    history_.push_back({std::move(next), std::move(result)});
    return true;
}

bool Session::back()
{
    if (!canBack()) return false;
    history_.pop_back();
    return true;
}

void Session::restart()
{
    history_.resize(1);
}

const script::Command* Session::currentCommand() const noexcept
{
    return canBack() ? &commands_[step() - 1] : nullptr;
}

const script::CommandResult* Session::currentResult() const noexcept
{
    return canBack() ? &*history_.back().result : nullptr;
}

} // namespace memsim::visual

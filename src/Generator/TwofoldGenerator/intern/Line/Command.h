// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include <functional>
#include <map>
#include <string>
#include <string_view>

namespace Twofold::intern {

struct FileLine;

struct LineCommand {
    FileLine const& line;
    std::string_view::iterator begin, end;
};
enum class LineCommandResult { Success, Error, Fallback };

namespace Line {

/// @brief line processing for commands.
class Command {
public:
    using Function = std::move_only_function<LineCommandResult(LineCommand const&)>;
    using Map = std::map<std::string_view, Function>;

public:
    Command(Map map, Function fallback)
        : m_map{std::move(map)}
        , m_fallback{std::move(fallback)} {}

    void operator()(FileLine const& line);

private:
    Map m_map;
    Function m_fallback;
};

} // namespace Line
} // namespace Twofold::intern

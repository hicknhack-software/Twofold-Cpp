/* Twofold-Cpp
 * (C) Copyright 2025 HicknHack Software GmbH
 *
 * The original code can be found at:
 *     https://github.com/hicknhack-software/Twofold-Cpp
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
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

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
#include "Passthrough.h"

#include "TwofoldGenerator/intern/CodeBuilder.h"
#include "TwofoldGenerator/intern/Line/Command.h"

namespace Twofold::intern::Command {

auto Passthrough::operator()(LineCommand const& command) const -> LineCommandResult {
    m_builder << OriginScript{.text = {command.line, TextSpan{command.line.begin, command.line.end}}};
    return LineCommandResult::Success;
}

} // namespace Twofold::intern::Command

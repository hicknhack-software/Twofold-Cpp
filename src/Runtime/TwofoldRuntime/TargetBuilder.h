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
#include "TwofoldRuntime/intern/SourceMapTextBuilder.h"

#include <string>
#include <string_view>
#include <vector>

namespace TwofoldRuntime {

using intern::SourceMapText;

/// @brief Target Builder
class TargetBuilder {
public:
    inline auto build() const -> SourceMapText { return m_sourceMapBuilder.build(); }

    void append(std::string_view text, FilePosition originPosition);
    void appendExpression(std::string_view text, FilePosition originPosition);
    void newLine(FilePosition originPosition);

    void pushIndentation(std::string_view indent, FilePosition originPosition);
    void popIndentation();

    void indentPart(std::string_view indent, FilePosition originPosition);
    void pushPartIndent(FilePosition originPosition);
    void popPartIndent();

private:
    intern::SourceMapTextBuilder m_sourceMapBuilder;
    std::string m_partIndent;
    std::vector<std::pair<std::string, std::string>> m_indentationStack;
};

} // namespace TwofoldRuntime

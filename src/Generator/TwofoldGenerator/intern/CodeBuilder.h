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
#include "TwofoldGenerator/intern/SourceMapTextBuilder.h"

#include <string>
#include <vector>

namespace Twofold::intern {

enum class OriginTextType { Part, Script, ScriptExpression, Target, IndentPart, PushIndention, PopIndention, NewLine };

template<OriginTextType>
struct OriginTextTemplate {
    OriginText text;
};

using OriginPart = OriginTextTemplate<OriginTextType::Part>;
using OriginScript = OriginTextTemplate<OriginTextType::Script>;
using OriginScriptExpression = OriginTextTemplate<OriginTextType::ScriptExpression>;
using OriginTarget = OriginTextTemplate<OriginTextType::Target>;
using IndentTargetPart = OriginTextTemplate<OriginTextType::IndentPart>;
using PushTargetIndentation = OriginTextTemplate<OriginTextType::PushIndention>;
using PopTargetIndentation = OriginTextTemplate<OriginTextType::PopIndention>;
using TargetNewLine = OriginTextTemplate<OriginTextType::NewLine>;

/// @brief data structure representing the prepared javascript
struct Code {
    std::string const content;
    SourceMapping const sourceMap;
};

/// @brief Builds template rendering javascript and source maps
/// builder class
class CodeBuilder {
public:
    auto build() const -> Code;

    inline auto indentation() const -> std::string_view { return m_sourceMapBuilder.indentation(); }
    inline void setIndentation(std::string indent) { m_sourceMapBuilder.setIndentation(std::move(indent)); }

    auto operator<<(OriginPart const&) -> CodeBuilder&;
    auto operator<<(OriginScript const&) -> CodeBuilder&;
    auto operator<<(OriginScriptExpression const&) -> CodeBuilder&;
    auto operator<<(OriginTarget const&) -> CodeBuilder&;

    auto operator<<(IndentTargetPart const&) -> CodeBuilder&;
    auto operator<<(PushTargetIndentation const&) -> CodeBuilder&;
    auto operator<<(PopTargetIndentation const&) -> CodeBuilder&;

    auto operator<<(TargetNewLine const) -> CodeBuilder&;

    auto operator<<(NewLine const) -> CodeBuilder&;

private:
    SourceMapTextBuilder m_sourceMapBuilder;
};

} // namespace Twofold::intern

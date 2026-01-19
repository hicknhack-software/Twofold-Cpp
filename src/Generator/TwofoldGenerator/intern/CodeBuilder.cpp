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
#include "CodeBuilder.h"

#include <format>

namespace Twofold::intern {
namespace {

auto escapeForCppString(std::string_view source) -> std::string {
    std::string out;
    out.reserve(static_cast<int>(std::distance(source.begin(), source.end())));
    for (auto c = source.begin(); c != source.end(); ++c) {
        switch (*c) {
        case '\\': out += "\\\\"; break;
        case '\r': out += "\\r"; break;
        case '\n': out += "\\n"; break;
        case '"': out += "\\\""; break;
        case '\'': out += "\\'"; break;
        default: out += *c; break;
        }
    }
    return out;
}

auto buildOriginText(
    FilePosition origin, int columnOffset, std::string_view span, Interpolation interpolation = Interpolation::None)
    -> OriginText {
    auto const originPosition = FilePosition{
        .name = origin.name,
        .position = {origin.position.line, origin.position.column + columnOffset},
    };
    return OriginText{originPosition, span, interpolation};
}

auto originPositionText(FilePosition position) -> std::string {
    return std::format(
        "TwofoldRuntime::FilePosition{{\"{}\", {{.line={}, .column={}}}}}",
        position.name.string(),
        position.position.line,
        position.position.column);
}

} // namespace

using namespace std::string_literals;

auto CodeBuilder::build() const -> Code {
    auto sourceMapText = m_sourceMapBuilder.build();
    return Code{sourceMapText.text, sourceMapText.sourceMap};
}

auto CodeBuilder::operator<<(OriginPart const& part) -> CodeBuilder& {
    m_sourceMapBuilder << part.text;
    return *this;
}

auto CodeBuilder::operator<<(OriginScript const& script) -> CodeBuilder& {
    m_sourceMapBuilder << script.text << NewLine();
    return *this;
}

auto CodeBuilder::operator<<(OriginScriptExpression const& expr) -> CodeBuilder& {
    if (0 == std::distance(expr.text.span.begin, expr.text.span.end)) return *this; // avoid empty script expressions

    auto const originLength = std::distance(expr.text.span.begin, expr.text.span.end);
    auto const originText = originPositionText(expr.text.origin);

    auto const prefix = std::format(
        "co_yield TwofoldRuntime::PushPartIndent{{{}}};co_yield TwofoldRuntime::AppendExpression{{", originText);
    auto const postfix = std::format(", {}}};co_yield TwofoldRuntime::PopPartIndent{{}};", originText);

    m_sourceMapBuilder << buildOriginText(expr.text.origin, -2, prefix, Interpolation::None); // #{
    m_sourceMapBuilder << expr.text;
    m_sourceMapBuilder << buildOriginText(expr.text.origin, originLength, postfix, Interpolation::None); // }
    m_sourceMapBuilder << NewLine();
    return *this;
}

auto CodeBuilder::operator<<(OriginTarget const& target) -> CodeBuilder& {
    if (0 == std::distance(target.text.span.begin, target.text.span.end)) return *this; // avoid empty text

    auto const originLength = std::distance(target.text.span.begin, target.text.span.end);
    auto const originText = originPositionText(target.text.origin);

    auto const prefix = "co_yield TwofoldRuntime::Append{\""s;
    auto const postfix = std::format("\", {}}};", originText);

    m_sourceMapBuilder << buildOriginText(target.text.origin, -1, prefix, Interpolation::None);
    m_sourceMapBuilder << OriginText{target.text.origin, TextSpan{escapeForCppString(target.text.span)}};
    m_sourceMapBuilder << buildOriginText(target.text.origin, originLength, postfix, Interpolation::None);
    m_sourceMapBuilder << NewLine();
    return *this;
}

auto CodeBuilder::operator<<(IndentTargetPart const& indent) -> CodeBuilder& {
    using namespace std::string_literals;

    auto const originLength = std::distance(indent.text.span.begin, indent.text.span.end);
    auto const originText = originPositionText(indent.text.origin);

    auto const prefix = "co_yield TwofoldRuntime::IndentPart{\""s;
    auto const postfix = std::format("\", {}}};", originText);

    m_sourceMapBuilder << buildOriginText(indent.text.origin, -1, prefix, Interpolation::None);
    m_sourceMapBuilder << OriginText{indent.text.origin, TextSpan{escapeForCppString(indent.text.span)}};
    m_sourceMapBuilder << buildOriginText(indent.text.origin, originLength, postfix, Interpolation::None);
    m_sourceMapBuilder << NewLine();
    return *this;
}

auto CodeBuilder::operator<<(PushTargetIndentation const& indent) -> CodeBuilder& {
    using namespace std::string_literals;
    auto const originLength = std::distance(indent.text.span.begin, indent.text.span.end);
    auto const originText = originPositionText(indent.text.origin);

    auto const prefix = "co_yield TwofoldRuntime::PushIndentation{\""s;
    auto const postfix = std::format("\", {}}};", originText);

    m_sourceMapBuilder << buildOriginText(indent.text.origin, -1, prefix, Interpolation::None);
    m_sourceMapBuilder << OriginText{indent.text.origin, TextSpan{escapeForCppString(indent.text.span)}};
    m_sourceMapBuilder << buildOriginText(indent.text.origin, originLength, postfix, Interpolation::None);
    m_sourceMapBuilder << NewLine();
    return *this;
}

auto CodeBuilder::operator<<(PopTargetIndentation const& indent) -> CodeBuilder& {
    using namespace std::string_literals;

    m_sourceMapBuilder << OriginText{
        indent.text.origin, TextSpan{"co_yield TwofoldRuntime::PopIndentation{};"s}, Interpolation::None};
    m_sourceMapBuilder << NewLine();
    return *this;
}

auto CodeBuilder::operator<<(TargetNewLine const newLine) -> CodeBuilder& {
    auto const originText = originPositionText(newLine.text.origin);
    auto const code = std::format("co_yield TwofoldRuntime::NewLine{{{}}};", originText);

    m_sourceMapBuilder << OriginText{newLine.text.origin, TextSpan{code}, Interpolation::None};
    m_sourceMapBuilder << NewLine();
    return *this;
}

auto CodeBuilder::operator<<(NewLine const) -> CodeBuilder& {
    m_sourceMapBuilder << NewLine();
    return *this;
}

} // namespace Twofold::intern

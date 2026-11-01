// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

#include "CodeBuilder.h"

#include <SourceMap/SourcePath.h>
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

} // namespace

auto CodeBuilder::originPositionText(FilePosition position) const -> std::string {
    return std::format(
        "TwofoldRuntime::FilePosition{{\"{}\", {{.line={}, .column={}}}}}",
        escapeForCppString(SourceMap::keyedPath(position.name, m_sourceRoot, m_sourceKey).string()),
        position.position.line,
        position.position.column);
}

void CodeBuilder::appendCppString(OriginText const& text) {
    auto remaining = std::string_view{text.span};
    auto origin = text.origin;
    while (!remaining.empty()) {
        auto escape = remaining.find_first_of("\\\r\n\"'");
        auto plain = remaining.substr(0, escape);
        if (!plain.empty()) {
            m_sourceMapBuilder << OriginText{origin, plain, Interpolation::OneToOne};
            origin.position.column += SourceMap::utf16Length(plain);
            remaining.remove_prefix(plain.size());
        }
        if (!remaining.empty()) {
            auto escaped = escapeForCppString(remaining.substr(0, 1));
            m_sourceMapBuilder << OriginText{origin, TextSpan{escaped}, Interpolation::None};
            ++origin.position.column;
            remaining.remove_prefix(1);
        }
    }
}

using namespace std::string_literals;

auto CodeBuilder::build() const -> Code {
    auto sourceMapText = m_sourceMapBuilder.build();
    auto data = sourceMapText.sourceMap.data();
    for (auto& entry : data.entries)
        entry.original.name = SourceMap::keyedPath(entry.original.name, m_sourceRoot, m_sourceKey);
    for (auto& caller : SourceMap::get<ExtCaller>(data))
        caller.original.name = SourceMap::keyedPath(caller.original.name, m_sourceRoot, m_sourceKey);
    return Code{sourceMapText.text, SourceMapping{std::move(data)}};
}

auto CodeBuilder::operator<<(OriginPart const& part) -> CodeBuilder& {
    m_sourceMapBuilder << part.text;
    return *this;
}

auto CodeBuilder::operator<<(OriginScript const& script) -> CodeBuilder& {
    m_sourceMapBuilder << script.text << NewLine();
    return *this;
}

auto CodeBuilder::operator<<(OriginScriptCall const& script) -> CodeBuilder& {
    m_sourceMapBuilder << script.text;
    if (script.text.span.end == script.text.span.begin || script.text.span.end[-1] != ';') {
        static constexpr auto semicolon = std::string_view{";"};
        auto const originLength = SourceMap::utf16Length(std::string_view{script.text.span});
        m_sourceMapBuilder << buildOriginText(script.text.origin, originLength, semicolon, Interpolation::None); // ;
    }
    m_sourceMapBuilder << NewLine();
    return *this;
}

auto CodeBuilder::operator<<(OriginScriptExpression const& expr) -> CodeBuilder& {
    if (0 == SourceMap::utf16Length(std::string_view{expr.text.span})) return *this; // avoid empty script expressions

    auto const originLength = SourceMap::utf16Length(std::string_view{expr.text.span});
    auto const originText = originPositionText(expr.text.origin);

    static constexpr auto coawaitView = std::string_view{"co_await"};
    if (std::search(expr.text.span.begin, expr.text.span.end, coawaitView.begin(), coawaitView.end()) !=
        expr.text.span.end) {
        auto const push = std::format("co_yield TwofoldRuntime::PushCaller{{{}}};", originText);
        m_sourceMapBuilder << buildOriginText(expr.text.origin, -2, push);
        m_sourceMapBuilder << expr.text;
        if (expr.text.span.end == expr.text.span.begin || expr.text.span.end[-1] != ';') {
            static constexpr auto semicolon = std::string_view{";"};
            m_sourceMapBuilder << buildOriginText(expr.text.origin, originLength, semicolon, Interpolation::None); // ;
        }
        m_sourceMapBuilder << buildOriginText(expr.text.origin, originLength, "co_yield TwofoldRuntime::PopCaller{};");
        m_sourceMapBuilder << NewLine();
        return *this;
    }
    else {
        auto const prefix = std::format(
            "co_yield TwofoldRuntime::PushPartIndent{{{}}};co_yield TwofoldRuntime::AppendExpression{{TWOFOLD_EXPR(",
            originText);
        auto const postfix = std::format("), {}}};co_yield TwofoldRuntime::PopPartIndent{{}};", originText);

        m_sourceMapBuilder << buildOriginText(expr.text.origin, -2, prefix, Interpolation::None); // #{
        m_sourceMapBuilder << expr.text;
        m_sourceMapBuilder << buildOriginText(expr.text.origin, originLength, postfix, Interpolation::None); // }
    }
    m_sourceMapBuilder << NewLine();
    return *this;
}

auto CodeBuilder::operator<<(OriginTarget const& target) -> CodeBuilder& {
    if (0 == SourceMap::utf16Length(std::string_view{target.text.span})) return *this; // avoid empty text

    auto const originLength = SourceMap::utf16Length(std::string_view{target.text.span});
    auto const originText = originPositionText(target.text.origin);

    auto const prefix = "co_yield TwofoldRuntime::Append{\""s;
    auto const postfix = std::format("\", {}}};", originText);

    m_sourceMapBuilder << buildOriginText(target.text.origin, -1, prefix, Interpolation::None);
    appendCppString(target.text);
    m_sourceMapBuilder << buildOriginText(target.text.origin, originLength, postfix, Interpolation::None);
    m_sourceMapBuilder << NewLine();
    return *this;
}

auto CodeBuilder::operator<<(IndentTargetPart const& indent) -> CodeBuilder& {
    using namespace std::string_literals;

    auto const originLength = SourceMap::utf16Length(std::string_view{indent.text.span});
    auto const originText = originPositionText(indent.text.origin);

    auto const prefix = "co_yield TwofoldRuntime::IndentPart{\""s;
    auto const postfix = std::format("\", {}}};", originText);

    m_sourceMapBuilder << buildOriginText(indent.text.origin, -1, prefix, Interpolation::None);
    appendCppString(indent.text);
    m_sourceMapBuilder << buildOriginText(indent.text.origin, originLength, postfix, Interpolation::None);
    m_sourceMapBuilder << NewLine();
    return *this;
}

auto CodeBuilder::operator<<(PushTargetIndentation const& indent) -> CodeBuilder& {
    using namespace std::string_literals;
    auto const originLength = SourceMap::utf16Length(std::string_view{indent.text.span});
    auto const originText = originPositionText(indent.text.origin);

    auto const prefix = "co_yield TwofoldRuntime::PushIndentation{\""s;
    auto const postfix = std::format("\", {}}};", originText);

    m_sourceMapBuilder << buildOriginText(indent.text.origin, -1, prefix, Interpolation::None);
    appendCppString(indent.text);
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

auto CodeBuilder::operator<<(TargetText const& targetText) -> CodeBuilder& {
    m_sourceMapBuilder << targetText;
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

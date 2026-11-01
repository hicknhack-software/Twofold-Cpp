// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include "TwofoldGenerator/intern/SourceMapTextBuilder.h"

#include <string>
#include <vector>

namespace Twofold::intern {

enum class OriginTextType {
    Part,
    Script,
    ScriptCall,
    ScriptExpression,
    Target,
    IndentPart,
    PushIndention,
    PopIndention,
    NewLine
};

template<OriginTextType>
struct OriginTextTemplate {
    OriginText text;
};

using OriginPart = OriginTextTemplate<OriginTextType::Part>;
using OriginScript = OriginTextTemplate<OriginTextType::Script>;
using OriginScriptCall = OriginTextTemplate<OriginTextType::ScriptCall>;
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
    CodeBuilder() = default;
    CodeBuilder(std::filesystem::path root, std::string key)
        : m_sourceRoot(std::move(root))
        , m_sourceKey(std::move(key)) {}
    auto build() const -> Code;

    inline auto indentation() const -> std::string_view { return m_sourceMapBuilder.indentation(); }
    inline void setIndentation(std::string indent) { m_sourceMapBuilder.setIndentation(std::move(indent)); }

    auto operator<<(OriginPart const&) -> CodeBuilder&;
    auto operator<<(OriginScript const&) -> CodeBuilder&;
    auto operator<<(OriginScriptCall const&) -> CodeBuilder&;
    auto operator<<(OriginScriptExpression const&) -> CodeBuilder&;
    auto operator<<(OriginTarget const&) -> CodeBuilder&;

    auto operator<<(IndentTargetPart const&) -> CodeBuilder&;
    auto operator<<(PushTargetIndentation const&) -> CodeBuilder&;
    auto operator<<(PopTargetIndentation const&) -> CodeBuilder&;

    auto operator<<(TargetText const&) -> CodeBuilder&;
    auto operator<<(TargetNewLine const) -> CodeBuilder&;

    auto operator<<(NewLine const) -> CodeBuilder&;

private:
    auto originPositionText(FilePosition position) const -> std::string;
    void appendCppString(OriginText const& text);
    std::filesystem::path m_sourceRoot;
    std::string m_sourceKey = "template";
    SourceMapTextBuilder m_sourceMapBuilder;
};

} // namespace Twofold::intern

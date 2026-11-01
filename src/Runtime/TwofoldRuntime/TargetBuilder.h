// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include "TwofoldRuntime/intern/SourceMapTextBuilder.h"

#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace TwofoldRuntime {

using intern::SourceMapText;

/// @brief Target Builder
class TargetBuilder {
public:
    inline auto build() const -> SourceMapText { return m_sourceMapBuilder.build(); }

    using SourceResolver = std::function<FilePosition(char const*)>;
    void setSourceResolver(SourceResolver resolver) { m_sourceResolver = std::move(resolver); }
    auto resolveSource(char const* location) const -> FilePosition {
        return location && m_sourceResolver ? m_sourceResolver(location) : FilePosition{};
    }
    void pushCaller(FilePosition origin) { m_sourceMapBuilder.pushCaller(origin); }
    void popCaller() { m_sourceMapBuilder.popCaller(); }

    auto pushSource(FilePosition origin) -> bool;
    void popSource();

    void append(std::string_view text, FilePosition originPosition);
    void appendExpression(std::string_view text, FilePosition originPosition);
    void newLine(FilePosition originPosition);

    void pushIndentation(std::string_view indent, FilePosition originPosition);
    void popIndentation();

    void indentPart(std::string_view indent, FilePosition originPosition);
    void pushPartIndent(FilePosition originPosition);
    void popPartIndent();

private:
    SourceResolver m_sourceResolver;
    std::vector<FilePosition> m_sourceOrigins;
    intern::SourceMapTextBuilder m_sourceMapBuilder;
    std::string m_partIndent;
    std::vector<std::pair<std::string, std::string>> m_indentationStack;
};

} // namespace TwofoldRuntime

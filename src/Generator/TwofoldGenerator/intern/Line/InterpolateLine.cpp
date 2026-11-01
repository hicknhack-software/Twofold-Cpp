// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

#include "InterpolateLine.h"

#include "TwofoldGenerator/intern/CodeBuilder.h"

namespace Twofold::intern::Line {

void InterpolateLine::operator()(FileLine const& line) const {
    this->Interpolation::operator()(line);
    m_builder << TargetNewLine{{line, TextSpan{line.end, line.end}}};
}

} // namespace Twofold::intern::Line

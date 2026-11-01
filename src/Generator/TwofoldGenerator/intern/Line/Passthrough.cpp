// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

#include "Passthrough.h"

#include "TwofoldGenerator/intern/CodeBuilder.h"
#include "TwofoldGenerator/intern/LineProcessor.h"

namespace Twofold::intern::Line {

void Passthrough::operator()(FileLine const& line) const {
    m_builder << OriginScript{.text = {line, TextSpan{line.begin, line.end}}};
}

} // namespace Twofold::intern::Line

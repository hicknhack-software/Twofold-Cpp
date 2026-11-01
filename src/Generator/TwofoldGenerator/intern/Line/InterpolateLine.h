// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include "Interpolation.h"

namespace Twofold::intern::Line {

/// @brief The same as the Interpolation class, except
/// the added newline at the end of the processing.
class InterpolateLine : public Interpolation {
public:
    using Interpolation::Interpolation;

    void operator()(FileLine const&) const;
};

} // namespace Twofold::intern::Line

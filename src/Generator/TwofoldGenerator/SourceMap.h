// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include "SourceMap/Extension/Caller.h"
#include "SourceMap/Extension/Interpolation.h"
#include "SourceMap/Mapping.h"

namespace Twofold {

// The SourceMap type specialization for Twofold

using FilePosition = SourceMap::FilePosition;

using ExtInterpolation = SourceMap::Extension::Interpolation;
using Interpolation = SourceMap::Interpolation;

using ExtCaller = SourceMap::Extension::Caller;
using Caller = SourceMap::Caller;
using CallerList = SourceMap::CallerList;
using CallerIndex = SourceMap::CallerIndex;

using SourceMapping = SourceMap::Mapping<ExtInterpolation, ExtCaller>;
using SourceData = SourceMapping::Data;
using SourceEntry = SourceMapping::Entry;

} // namespace Twofold

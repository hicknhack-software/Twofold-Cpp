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
#include "SourceMap/Extension/Caller.h"
#include "SourceMap/Extension/Interpolation.h"
#include "SourceMap/Mapping.h"

namespace TwofoldRuntime {

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

} // namespace TwofoldRuntime

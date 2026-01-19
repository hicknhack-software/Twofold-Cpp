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
#include "TwofoldGenerator/intern/LineProcessor.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <string>
#include <vector>

namespace {

struct TestData {
    std::string templateText;
    std::vector<std::string> expectedSpaceStrings;
    std::vector<std::string> expectedNonSpaceStrings;
};

} // namespace

TEST_CASE("TestLines", "[lines]") {
    auto const [templateText, expectedSpaceStrings, expectedNonSpaceStrings] = GENERATE(
        TestData{"", {}, {}},
        TestData{" \t\n", {" \t"}, {""}},
        TestData{" \tFirst line \t", {" \t"}, {"First line \t"}},
        TestData{" \tFirst line\n \tSecond line \t", {" \t", " \t"}, {"First line", "Second line \t"}});

    auto actualSpaceStrings = std::vector<std::string>{};
    auto actualNonSpaceStrings = std::vector<std::string>{};

    auto const lineFunc = [&](Twofold::intern::FileLine const& line) {
        auto const spaceString = std::string(line.begin, line.firstNonSpace);
        auto const nonSpaceString = std::string(line.firstNonSpace, line.end);

        actualSpaceStrings.push_back(spaceString);
        actualNonSpaceStrings.push_back(nonSpaceString);
    };

    auto processor = Twofold::intern::LineProcessor{{}, lineFunc};
    processor(std::filesystem::path{"testLines"}, templateText);

    CHECK(actualSpaceStrings == expectedSpaceStrings);
    CHECK(actualNonSpaceStrings == expectedNonSpaceStrings);
}

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
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <iostream>
#include <string>
#include <vector>

struct Type {
    bool isArray;
    std::string name;
    std::string text;
};

struct Data {
    Type type;
    std::vector<std::string> baseNames;
    std::string name;
};

#include "benchmark.hpp"

TEST_CASE("Benchmark Twofold Generation", "[benchmark]") {
    auto data = Data{
        .type = Type{
            .isArray = true,
            .name = "TestArray",
            .text = "Some text"
        },
        .baseNames = {"base1", "base2", "base3"},
        .name = "Main"
    };

    auto start = std::chrono::high_resolution_clock::now();

    auto g = generate(data);
    auto builder = TwofoldBuilder{};
    g.setBuilder(&builder);
    g.handle.resume();
    auto result = builder.build();

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Generation took: " << duration.count() << "ms" << std::endl;
    
    // Optional: verify result not empty
    CHECK(!result.text.empty());
}

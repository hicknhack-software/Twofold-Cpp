// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

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
        .type = Type{.isArray = true, .name = "TestArray", .text = "Some text"},
        .baseNames = {"base1", "base2", "base3"},
        .name = "Main"};

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

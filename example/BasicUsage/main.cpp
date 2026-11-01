// SPDX-FileCopyrightText: 2026 HicknHack Software GmbH
// SPDX-License-Identifier: Apache-2.0

#include <string>
#include <vector>

struct Method {
    std::string name;
    std::vector<std::string> args;
    std::string body;
};
struct Data {
    std::string name;
    std::vector<std::string> args;
    std::vector<Method> methods;
};

#include "generate.hpp"
#include <cstdio>

int main() {
    auto data = Data{
        .name = "TwofoldGenerated",
        .args = {},
        .methods =
            {
                Method{
                    .name = "hello",
                    .args = {"greeted"},
                    .body = "console.log('Hello ' + greeted);",
                },
            },
    };
    auto g = generate(data);
    auto builder = TwofoldBuilder{};
    g.setBuilder(&builder);
    g.handle.resume();

    std::puts(builder.build().text.data());
}

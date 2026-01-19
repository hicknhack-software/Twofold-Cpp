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

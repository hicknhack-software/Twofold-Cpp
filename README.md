# Twofold-Cpp

**Generate correctly indented source code using just C++**

This is the C++ implementation of Twofold.
We also have a Qt/Javascript and pure Javascript based implementation for NodeJS.

## What is the best method to generate source code?

* more difficult than regular html. It requires tight controls about every whitespace and tools for indentation.
* often requires more control structures in order to access models or AST-like structures.

We faced these challenges and Twofold is the result.

## Features

* indentation handling
* source maps for generated javascript
  * javascript exceptions and syntax errors will point into your template
* nested source maps for generated targets
  * positions of all the calls that lead to any target character
* control of every generated line and every whitespace
* fully unit and integration tested

## Requirements

* C++23 compiler
  * Tested with GCC 15.2 and Clang 21
  * Tested with Visual Studio 2026 Community Edition Compiler
  * Other compilers might work, but are not currently tested
* Conan
  * `conan profile detect`
  * `conan install . -g=QbsDeps --output-folder=R:\twofold-debug --build missing --settings=build_type=Debug`
  * `conan install . -g=QbsDeps --output-folder=R:\twofold-release --build missing --settings=build_type=Release`
  * VsCode: `"moduleProviders.conan.installDirectory": "R:\\twofold-debug"` to `.vscode/qbs-cconfiguration.json` and same for release
  * QtCreator: `moduleProviders.conan.installDirectory:R:\twofold-debug` to qbs build arguments and same for release

## How does it work?

Twofold is basically C++ that is extended with constructs to generate source code output.

```cpp.twofold
  \ indented output #{"interpolated"} without a line break
  | indented output #{"interpolated"} with a line break
  # include "indent_the_included_file.h.twofold"
  = change_indentation_of_cpp()
// every line is regular C++
```

These control characters have to be the first non-whitespace character on each line.
Every whitespace before them is ignored.
Every whitespace after is used as the indentation. Indentation is cumulative and added to each generated line.

**Hint:** Use the whitespaces before control characters to indent them in a column deeper than any surrounding C++.
This gives you a visual splitter.

This is basically everything you need to know.

## Example Template

```twofold
// file included.hpp.twofold -> generates included.hpp
#include "TwofoldRuntime/Twofold.h"

auto methodArgs(auto const& args) -> Twofold {
  auto isFirst = true;
  for (auto& arg : args) {
    if (!isFirst) {
        // use interpolation to preserve the whitespace
        \#{', '}
    }
    isFirst = false;
    // use backslash to avoid linebreaks
        \#{arg}
  }
}
auto showMethod(auto const& method) -> Twofold {
        |function #{method.name}(#{methodArgs(method.args)}) {
        |  #{method.body}
        |}
}
```

```twofold
// file generate.hpp.twofold -> generates generate.hpp
#include "TwofoldRuntime/Twofold.h"
#include "included.hpp.twofold"

auto generate(Data const& data) -> Twofold {
        |function #{data.name}Class(#{methodArgs(data.args)}) {
  for (auto& method : data.methods) {
        =  showMethod(method);
  }
        |
        |  return {
  for (auto& method : data.methods) {
        |    "#{method.name}": #{method.name},
  }
        |  };
        |}
}
```

The twofold-generator will generate the regular C++ code.
And we can use them like this:

```cpp
// file main.cpp
#include <vector>
#include <string>

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

int main() {
  auto data = Data{
    .name = "TwofoldGenerated",
    .args = {},
    .methods = {
      Method{
        .name = "hello",
        .args = { "greeted" },
        .body = "console.log('Hello ' + greeted);",
      },
    },
  };
  auto g = generate(data);
  auto builder = Twofold::Builder{};
  g.setBuilder(&builder);
  g.run();

  std::puts(builder.text.data());
}
```

Running the program will generate the following output.

```javascript
function TwofoldGeneratedClass() {
  function hello(greeted) {
    console.log('Hello ' + greeted);
  }

  return {
    "hello": hello,
  };
}
```

## Roadmap

Twofold is simple. We find it very useful as it is.

* a way to create multiline comments after a statement

  ````javascript
  function hello() { # just an example
                     # multi line comment
                     # ...
    console.log("indented");
  }
  ````

* feel free to add your wish as an issue

## License

Apache License Version 2.0
See LICENSE file for more details

## Contribute

If you like this project give it a star.
If you don't like it or found a bug, please write an issue.

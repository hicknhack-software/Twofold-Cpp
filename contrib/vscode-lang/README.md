# Twofold Language Support

Syntax highlighting for the [Twofold](https://github.com/user/twofold-cpp) template language in Visual Studio Code.

## Features

- Syntax highlighting for `.twofold` files
- Specialized highlighting for C++ target files (`.hpp.twofold`, `.cpp.twofold`, etc.)
- Support for all Twofold directives:
  - `|` - Output line with line break
  - `\` - Output line without line break
  - `# include` - Include another template file
  - `=` - Indentation change directive
- Interpolation highlighting with `#{...}` syntax
- Embedded C++ syntax highlighting in interpolation and output lines

## Supported File Extensions

### Generic Twofold
- `.twofold`

### C++ Target
- `.hpp.twofold`
- `.cpp.twofold`
- `.h.twofold`
- `.c.twofold`
- `.hxx.twofold`
- `.cxx.twofold`
- `.cc.twofold`
- `.hh.twofold`

## Syntax Overview

Twofold is a C++ template language for generating source code. Control characters must be the first non-whitespace character on each line:

```cpp.twofold
  \ indented output #{"interpolated"} without a line break
  | indented output #{"interpolated"} with a line break
  # include "indent_the_included_file.h.twofold"
  = change_indentation_of_cpp()
// every line is regular C++
```

## Installation

### From VSIX
1. Download the `.vsix` file
2. Open VS Code
3. Go to Extensions (Ctrl+Shift+X)
4. Click the "..." menu and select "Install from VSIX..."

### From Source
```bash
git clone https://github.com/user/twofold-cpp.git
cd twofold-cpp/contrib/vscode-lang
npm install
npm run package
code --install-extension twofold-0.1.0.vsix
```

## License

Apache License Version 2.0
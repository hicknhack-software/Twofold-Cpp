# Twofold Language Support

Syntax highlighting, formatting, and source map navigation for the [Twofold](https://github.com/hicknhack-software/Twofold-Cpp) template language in Visual Studio Code.

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
- A **Twofold** panel listing all mapped sources and the generation stack at the output cursor
- Clickable source and caller entries that open and highlight their generating section
- **Go to Definition** (F12), **Peek Definition** (Alt+F12), and Ctrl/Cmd-hover previews in generated files
- Hover snippets with the template/input caller stack

## Source map navigation

Open any generated text file containing a source map comment, for example:

```cpp
//# sourceMappingURL=generated.hpp.map
```

HTML `<!--# sourceMappingURL=generated.html.map -->` and CSS `/*# sourceMappingURL=generated.css.map */` comments are also recognized.

The extension activates for generated files in any language, even when no `.twofold` file is open.
Open **View > Open View... > Sources and Generation Stack** to show the Twofold panel.
Move the cursor in the generated output to update **Generating Source** and **Stack Trace**.
The stack is listed from the innermost caller to the outermost, including input scopes when recorded.
The **Sources** group includes every source in the map, including files referenced only by callers.
Click any source or stack frame to open it. The originating output's panel stays visible while you inspect that source.

Use **Twofold: Go to Generating Source** from the Command Palette for direct navigation with a highlighted source section.
F12, Alt+F12, and Ctrl/Cmd-hover use VS Code's native definition and inline peek UI alongside other language extensions.
For one-to-one segments, the highlighted section covers the mapped output fragment; computed output uses its recorded source position and the surrounding token or interpolation.
Source maps record starting positions rather than complete expression ranges, so a computed expression's full extent is not always available.

Relative map references resolve beside the generated file. Source paths resolve beside the map, honoring its `sourceRoot`.
Absolute paths, `file:` URLs, percent-encoded paths, and inline `data:application/json` maps are supported.
The extension reads flat revision 3 maps and the SourceMap-Cpp `x_de_hicknhack_software_column_interpolation` and `x_de_hicknhack_software_callstack` extensions.
It does not fetch maps or sources over HTTP. Maps with indexed `sections` are not supported.

### Workspace discovery and external sources

Source mappings work without configuration when the sources are in the workspace.
The extension searches every workspace folder, including excluded test/build folders.
It first matches the source path suffix, then falls back to the filename if the source tree has moved.
One match opens directly. Multiple matches are all available in F12/Peek Definition, hover links, and expandable entries in the Sources panel.
The direct navigation command shows a picker instead of selecting an arbitrary match.
Existing standard relative/absolute source references are used directly when they resolve.

Hover a generated section and choose **Configure source mapping…** beside the generating source or an input stack frame.
The same action is available in source-entry tooltips and through **Twofold: Configure Source Mapping** in the Command Palette.
Choose whether the binding applies to this generated file, its directory (including subdirectories), this workspace folder, or globally.
Then enter a source prefix such as `template://` or `protlr3://` and select the external source root.
Enter the complete source name to select a single source file instead of a directory.
The hover action keeps the generated file's context even after navigating into a source.

Global/workspace bindings use `twofold.sourceMap.roots`; file/directory bindings use `twofold.sourceMap.bindings`.
For example:

```json
{
  "twofold.sourceMap.roots": {
    "template://": "/external/TwofoldTemplates"
  },
  "twofold.sourceMap.bindings": {
    "file:///build/test-a": {
      "scope": "directory",
      "roots": { "protlr3://": "/external/test-a/sources" }
    },
    "file:///build/test-a/generated.hpp": {
      "scope": "file",
      "roots": { "protlr3://packet.protlr3": "/external/special/packet.protlr3" }
    }
  }
}
```

Explicit bindings take precedence over discovery: generated file, nearest generated directory, ancestor directories, then general roots.
Within each scope the longest source prefix wins; unmatched prefixes fall through to broader scopes.
File/directory bindings created by the tooltip are saved in workspace-folder settings, or user settings when no workspace is open.
The global option writes user settings. Workspace settings continue to override global settings according to VS Code's usual rules.
Binding keys are generated file/directory URIs, including `vscode-remote:` URIs for remote workspaces.
Source roots can be paths or URIs; relative paths use the generated file's workspace folder, or the first folder for output outside the workspace.

If no local source is found, embedded `sourcesContent` snapshots open in a read-only virtual document when available.
A missing source still has a configuration link in the hover. A broken explicit binding reports its configured path so it can be corrected.
Discovery refreshes when workspace files are created/deleted or folders change.

Maps reload when their files change, including outside the workspace, and when an open map is edited.
Use the panel's refresh button or **Twofold: Refresh Source Map** after regenerating if file notifications are unavailable.
Malformed or missing maps appear as a message in the panel; normal editing and existing highlighting/formatting remain available.

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
git clone https://github.com/hicknhack-software/Twofold-Cpp.git
cd twofold-cpp/contrib/vscode-lang
npm install
npm run package
code --install-extension twofold-0.3.1.vsix
```

## Development and verification

```bash
npm ci
npm test
npm run package
```

`npm test` compiles the extension and checks source map references, inline maps, UTF-16 interpolation, unmapped regions, caller deltas, and malformed maps.
`npm run package` compiles automatically before producing the VSIX.

To also exercise navigation in a real VS Code extension host, set `VSCODE_EXECUTABLE` to an installed VS Code executable and run `npm run test:extension`.
For example, in PowerShell:

```powershell
$env:VSCODE_EXECUTABLE = "$env:LOCALAPPDATA/Programs/Microsoft VS Code/Code.exe"
npm run test:extension
```

This launches an isolated test window with temporary workspace/settings, without using your normal VS Code profile.
It verifies provider registration, unique/ambiguous workspace discovery, definition and hover matches, tooltip actions, global/file/directory binding precedence, exact-file overrides, embedded content, map edits, and file watcher recovery.
Failed test workspaces and their log are retained at the path printed by the runner.

To check actual generated output, set `TWOFOLD_REAL_FIXTURES` to a directory containing a `manifest.json` with source bindings, artifact paths, and navigation cases.
The parent Protlr3 checkout provides `scripts/prepare_source_map_navigation_test.py` to create these fixtures using the actual Protlr3 and Twofold executables.
With that variable set, both `npm test` and `npm run test:extension` verify the generated maps; host checks also follow definitions and click input stack frames in C++, C++ modules, HTML, and generated template C++.

## License

Apache License Version 2.0

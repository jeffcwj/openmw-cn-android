# AGENTS.md — OpenMW Coding Agent Reference

## Project Overview

OpenMW is a C++20 open-source reimplementation of the Morrowind game engine. It uses CMake, OpenSceneGraph, Bullet Physics, MyGUI, Lua (via sol3), Boost, SDL2, and FFmpeg. The codebase lives in two main trees:

- `apps/` — Application targets (openmw, opencs, launcher, tools, tests, benchmarks)
- `components/` — Shared library code (ESM loading, rendering, navigation, scripting, etc.)
- `extern/` — Vendored third-party code (do NOT modify or reformat)
- 
## 语言规范

- **语言**：文档、注释、回答均用中文
- **新文件**：优先 Kotlin 而非 Java

## Build Commands

```bash
# Configure (out-of-source build required)
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo

# Build everything
cmake --build . -- -j$(nproc)          # Linux/macOS
cmake --build . --config RelWithDebInfo # Windows/MSVC

# Build a single target
cmake --build . --target openmw
cmake --build . --target components-tests
```

### CMake Options (key ones)

| Option                    | Default | Purpose                        |
|---------------------------|---------|--------------------------------|
| `BUILD_COMPONENTS_TESTS`  | OFF     | Build components test suite    |
| `BUILD_OPENMW_TESTS`      | OFF     | Build openmw test suite        |
| `BUILD_OPENCS_TESTS`      | OFF     | Build construction set tests   |
| `BUILD_BENCHMARKS`        | OFF     | Build Google Benchmark suites  |
| `CMAKE_BUILD_TYPE`        | RelWithDebInfo | Debug, Release, RelWithDebInfo, MinSizeRel |

### Configure with tests enabled

```bash
cmake .. -DBUILD_COMPONENTS_TESTS=ON -DBUILD_OPENMW_TESTS=ON -DBUILD_OPENCS_TESTS=ON -DBUILD_BENCHMARKS=ON
```

## Testing

**Framework**: Google Test + Google Mock (GTest/GMock).

**Test executables** (in build directory):
- `components-tests` — tests for the components library
- `openmw-tests` — tests for the openmw game engine
- `openmw-cs-tests` — tests for OpenMW Construction Set

```bash
# Run all tests for a suite
./components-tests
./openmw-tests
./openmw-cs-tests

# Run a single test by name (GTest filter)
./components-tests --gtest_filter="*LuaState*"
./openmw-tests --gtest_filter="MWWorldStoreTest.*"

# Run with XML output (CI style)
./components-tests --gtest_output="xml:components-tests.xml"
```

Test source files live in:
- `apps/components_tests/` — organized by component (lua/, esm/, misc/, etc.)
- `apps/openmw_tests/` — organized by module (mwworld/, mwdialogue/, mwscript/)
- `apps/opencs_tests/` — model/world tests

## Linting & Formatting

### clang-format (enforced in CI)

A `.clang-format` config exists at the repo root. CI runs `clang-format-14`.

```bash
# Check formatting (dry run)
clang-format --dry-run -Werror <file.cpp>

# Format a file in-place
clang-format -i <file.cpp>

# CI check script (excludes extern/)
CI/check_clang_format.sh
```

Key formatting rules:
- **Column limit**: 120
- **Indent**: 4 spaces, no tabs
- **Braces**: Allman style (opening brace on new line for classes, functions, control flow, namespaces)
- **Namespace indentation**: All (content inside namespaces IS indented)
- **Pointer alignment**: Left (`int* ptr`, not `int *ptr`)
- **Short functions**: Inline only allowed on single line
- **Include sort**: CaseSensitive, blocks preserved

### clang-tidy (enforced in CI)

The `.clang-tidy` config enables:
- `portability-*`
- `clang-analyzer-*` (with some exclusions)
- `modernize-avoid-bind`
- `readability-identifier-naming` — Concepts must be CamelCase

All warnings are treated as errors. Header filter: `(apps|components)/`.

### File naming (enforced in CI)

Source files must match: `/[a-z0-9]+\.(cpp|hpp|h)$` — all lowercase alphanumeric.
Exceptions are listed in `CI/file_name_exceptions.txt`.

```bash
CI/check_file_names.sh
```

### CMake formatting

CMake files use spaces only (no tabs). Checked by `CI/check_cmake_format.sh`.

## Code Style Conventions

### C++ Standard

C++20 (`CMAKE_CXX_STANDARD 20`). Extensions disabled.

### Header Guards

Use traditional `#ifndef` / `#define` / `#endif` guards (NOT `#pragma once`):
```cpp
#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVIGATOR_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVIGATOR_H
// ...
#endif
```
Guard names follow the pattern: `OPENMW_` + path segments in uppercase + `_H`.
Older files may use `GAME_MWWORLD_ACTION_H` or `COMPONENTS_MISC_STRINGS_ALGORITHM_H`.

### Include Ordering

1. Corresponding header (`#include "foo.hpp"`)
2. Standard library headers (`<algorithm>`, `<string>`, etc.)
3. Third-party library headers (`<osg/...>`, `<SDL.h>`, `<boost/...>`)
4. Project component headers (`<components/...>`)
5. Local/app headers (`"mwinput/..."`, `"mwgui/..."`)

Groups are separated by blank lines. Includes within each block are sorted case-sensitively.

### Naming Conventions

| Element          | Convention     | Example                              |
|------------------|----------------|--------------------------------------|
| Namespaces       | PascalCase     | `MWWorld`, `DetourNavigator`, `Misc::StringUtils` |
| Classes/Structs  | PascalCase     | `CellStore`, `Navigator`, `ObjectShapes` |
| Functions        | camelCase      | `getRecordType()`, `executeImp()`, `ciEqual()` |
| Member variables | mPascalCase    | `mRefNum`, `mSoundId`, `mTarget`     |
| Local variables  | camelCase      | `newName`, `connectionStart`         |
| Constants        | camelCase or UPPER_SNAKE | `sRecordId`, `sSize`       |
| Static members   | sPascalCase    | `sDefaultWorldspaceId`, `sRecordId`  |
| Concepts         | CamelCase      | (enforced by clang-tidy)             |
| Enums            | PascalCase     | Values vary by context               |

### Namespace Usage

- All code lives in meaningful namespaces (`MWWorld`, `MWGui`, `ESM`, `DetourNavigator`, etc.)
- Namespace content is indented (configured in `.clang-format`: `NamespaceIndentation: All`)
- Nested namespaces use `::` syntax: `namespace Misc::StringUtils`
- Forward declarations in namespace blocks are common for decoupling

### Error Handling

- **Exceptions**: Used for fatal/unrecoverable errors (e.g., file not found, invalid data)
- **`Log(Debug::Error)`**: For non-fatal runtime errors — uses the `components/debug/debuglog.hpp` logging system
- **`assert()`**: Used for programmer errors / invariant violations
- Avoid empty catch blocks

### Smart Pointers & Memory

- `std::unique_ptr` for exclusive ownership
- `std::shared_ptr` where shared ownership is necessary
- `osg::ref_ptr<>` for OpenSceneGraph objects (OSG's reference counting)
- Raw pointers for non-owning references (common pattern in this codebase)

### Comments

- Doxygen-style `///` and `/** */` for API documentation
- `// comment` for inline explanations
- `/// \brief` for brief class/function descriptions
- `/// \param` for parameter docs

### Compiler Warnings (CI enforced)

CI builds with `-Werror` and these warnings enabled:
`-Wall -Wextra -Wundef -Wextra-semi -Wno-unused-parameter -pedantic -Wnon-virtual-dtor -Wunused`

GCC additionally: `-Wduplicated-branches -Wduplicated-cond -Wlogical-op`

## Project Structure Quick Reference

```
apps/
  openmw/          # Game engine (mwworld/, mwgui/, mwclass/, mwmechanics/, etc.)
  opencs/          # Construction Set editor
  launcher/        # Qt-based launcher
  components_tests/ # GTest tests for components/
  openmw_tests/    # GTest tests for openmw
  opencs_tests/    # GTest tests for opencs
  benchmarks/      # Google Benchmark suites
  esmtool/         # ESM file inspector CLI
  bsatool/         # BSA archive extractor CLI
components/
  esm/             # Elder Scrolls data format commons
  esm3/            # Morrowind ESM3 format loading
  esm4/            # ESM4 (Oblivion+) format loading
  detournavigator/ # Pathfinding / navmesh
  lua/             # Lua scripting engine
  vfs/             # Virtual filesystem
  resource/        # Asset/resource management
  settings/        # Settings system
  misc/            # Miscellaneous utilities
extern/            # Vendored third-party (DO NOT modify/reformat)
CI/                # CI scripts (format checks, build helpers)
files/             # Config files, data, shaders
```

## Key Reminders for Agents

1. **Do NOT touch `extern/`** — it has its own `.clang-tidy` that disables most checks.
2. **Run `clang-format`** on any file you modify (the CI will reject unformatted code).
3. **File names must be all lowercase** alphanumeric (check `CI/file_name_exceptions.txt` for historical exceptions).
4. **Match existing patterns** in the module you're editing — conventions vary slightly between older and newer code.
5. **Test changes** by building with `-DBUILD_COMPONENTS_TESTS=ON -DBUILD_OPENMW_TESTS=ON` and running the relevant test executable.
6. **Prefer minimal changes** — avoid reformatting code you didn't functionally change.
7. **GPLv3 licensed** — all contributions must be compatible.

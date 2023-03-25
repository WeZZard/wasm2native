# Get the Develop Environment Ready

Visual Studio Code is the recommended IDE for developing the project. If
you are experienced in configuring IDEs for multi-programming-languages
mixed develop environment which includes shell script, Python and C++,
you can also use them instead of Visual Studio Code.

The steps of configuring Visual Studio Code to fit the development process
of this project includes: 1) install the plugins; 2) build/install the
dependencies of the plugins and 3) configure the plugins.

## Install Visual Studio Code Plugins

- C-mantic by Tyler Dennis
- C/C++ Include Guard by Akira Miyakoda
- C++ TestMate by Mate Pek
- Clang-Format by Michael Johns
- clangd by LLVM
- CMake Language Support by Jose Torres
- CMake Tools by Microsoft
- CodeLLDB by Vadim Chugunov
- LLVM by Ingvar Stephanyan
- LLVM TableGen by Jakob Erzar
- Pylance by Microsoft
- Python by Microsoft
- WebAssembly by WebAssembly Foundation

## Build/Install the Dependencies of the Plugins

There are several dependencies of the plugins shall be installed:

- clang-foramt
- clang-tidy
- clangd

With the build script, we build a custom toolchain before building the
project itself. The dependencies we are going to installed are just there
in the toolchain's installation directory:
`wasm2native-project/toolchain/${HOST_TOOL_CONFIG}/${OS}-${ARCH}/bin`.

The variables in the path above are:

- `${HOST_TOOL_CONFIG}`: the config that used for building the toolchain
and host tools. Usually it would be `Release`.
- `${OS}`: the operating system of the host machine. Currently macOS is
the only supported operating system, thus you can fill `macOS` in it if
you are actually using macOS.
- `${ARCH}`: the processor architecture of the host machine. Since Apple
Silicon Mac is the only supported host machine at the moment, you can fill
`arm64` in it if you are using Apple Silicon Mac.

If you haven't build the project, you must build it first then configure
the Visual Studio Code Plugins to use dependencies in the custom
toolchain.

## Configure The Visual Studio Code and Plugins

Editor Behaviors

```json
{
  "editor.rulers": [80, 74],
  "editor.tabSize": 2,
  "editor.fontLigatures": false,
  "editor.inlayHints.enabled": "on",
  "editor.formatOnSave": true,
}
```

File Associations

`lit.cfg`, `lit.site.cfg.in` and `lit.local.cfg` is LLVM Integerated
Tester configs which are Python scripts indeed.

```json
{
  "files.associations": {
    "lit.cfg": "python",
    "lit.site.cfg.in": "python",
    "lit.local.cfg": "python"
  },
}
```

C++ TestMate

By configuring C++ TestMate, you can run and debug unit tests from Visual
Studio Code's Testing panel directly.

```json
{
  "testMate.cpp.test.executables": "../build/${GENERATOR}-${CONFIG}/wasm2native-${OS}-${ARCH}/**/*Tests",
}
```

- `${GENERATOR}` the build system generator that used by CMake. Currently
it is Ninja only.
- `${CONFIG}`: the config that used for building wasm2native. It would be
`Debug` if you ran the build script without any arguments.

LLDB

```json
{
  "lldb.displayFormat": "auto",
  "lldb.dereferencePointers": true,
  "lldb.consoleMode": "commands",
  "lldb.showDisassembly": "never",
}
```

clang-d

clang-d is much more intelligent than Microsoft C++ IntelliSense on non-
Windows platforms. By enalbing clang-d, you should **disable** Microsoft
C++ IntellSense on Visual Studio Code at the same time.

```json
{
  "clangd.path": "../toolchain/${HOST_TOOL_CONFIG}/${OS}-${ARCH}/bin/clangd",
  "clangd.arguments": [
    "--compile-commands-dir=../build/${GENERATOR}-${CONFIG}/wasm2native-${OS}-${ARCH}",
    "--query-driver=../toolchain/${HOST_TOOL_CONFIG}/${OS}-${ARCH}/bin/clang",
    "--all-scopes-completion",
    "--pch-storage=memory",
    "--background-index",
    "-j=10",
    "--clang-tidy",
    "--enable-config"
  ],
}
```

clang-format

```json
{
  "clang-format.executable": "../toolchain/${HOST_TOOL_CONFIG}/${OS}-${ARCH}/bin/clang-format",
}
```

C-mantic

```json
{
  "C_mantic.caseStyle": "camelCase",
  "C_mantic.extensions.headerFiles": ["h", "def"],
  "C_mantic.extensions.sourceFiles": ["c", "cpp"],
  "C_mantic.headerGuard.defineFormat": "W2N_${DIR}_${FILE_NAME}_${EXT}",
}
```

CMake Tools

By configuring CMake Tools, we can test our modifications on code with
Visual Studio Code's GUI.

Firstly, you have to get CMake's arguments that you use for generating the
build system of the project. You can use the `--dry-run` argument of the
build script to check these arguments out:

```shell
utils/build --dry-run
```

Then you can find a line like the following line in the outputs:

```shell
cmake -G Ninja  /SourceCache/wasm2native-project/wasm2native -DCMAKE_BUILD_TYPE:string=Debug -DCMAKE_C_COMPILER:string=/SourceCache/wasm2native-project/toolchain/Release/macosx-arm64/bin/clang -DCMAKE_CXX_COMPILER:string=/SourceCache/wasm2native-project/toolchain/Release/macosx-arm64/bin/clang++ -DLLVM_BUILD_ROOT:PATH=/SourceCache/wasm2native-project/build/Ninja-Debug/llvm-macosx-arm64 -DLLVM_SOURCE_ROOT:PATH=/SourceCache/wasm2native-project/llvm-project/llvm -DLLVM_DIR:PATH=/SourceCache/wasm2native-project/build/Ninja-Debug/llvm-macosx-arm64/lib/cmake/llvm -DLLVM_TARGETS_TO_BUILD:string= -DLLVM_BUILD_TESTS:bool=true -DClang_DIR:PATH=/SourceCache/wasm2native-project/build/Ninja-Debug/llvm-macosx-arm64/lib/cmake/clang -DW2N_MONOREPO_ROOT:PATH=/SourceCache/wasm2native-project -DW2N_SOURCE_ROOT:PATH=/SourceCache/wasm2native-project/wasm2native -DW2N_BUILD_ROOT:PATH=/SourceCache/wasm2native-project/build/Ninja-Debug/wasm2native-macosx-arm64 -DW2N_HOST_VARIANT_SDK=OSX -DW2N_HOST_VARIANT_ARCH=arm64 -DW2N_SDKS=OSX -DW2N_HOST_TOOLS_BIN_DIR=/SourceCache/wasm2native-project/toolchain/Release/macosx-arm64/bin
```

The contents behind `cmake -G Ninja  /SourceCache/wasm2native-project/wasm2native`
is the arguments that used by CMake for generating the build system. You
can just grab them and fill them in `cmake.configureArgs` line by line.

```json
{
  "cmake.buildDirectory": "${workspaceFolder}/../build/wasm2native-vscode-cmake",
  "cmake.configureArgs": [
    "-DCMAKE_BUILD_TYPE:string=Debug",
    "-DCMAKE_C_COMPILER:string=/SourceCache/wasm2native-project/toolchain/Release/macosx-arm64/bin/clang",
    // ...
  ],
}
```

C/C++ Include Guard Path

```json
{
  "C/C++ Include Guard.Path Depth": 0,
  "C/C++ Include Guard.Path Skip": 1,
  "C/C++ Include Guard.Spaces After Endif": 1,
  "C/C++ Include Guard.Macro Type": "Filepath",
  "C/C++ Include Guard.Header Extensions": [".h"],
  "C/C++ Include Guard.Comment Style": "Line",
  "C/C++ Include Guard.Remove Extension": false,
  "C/C++ Include Guard.Auto Update Path Allowlist": [
    "include",
    "lib",
    "tests",
    "tools"
  ],
}
```

Python

```json
{
  "python.formatting.autopep8Args": [
    "--indent-size=2",
    "--max-line-length=76",
    "--ignore=E121"
  ],
  "python.autoComplete.extraPaths": [
    "../llvm-project/llvm/utils/lit"
  ],
  "python.analysis.extraPaths": [
    "../llvm-project/llvm/utils/lit"
  ],
}
```

## Get Python Environment Ready

wasm2native uses pipenv to manage the version of Python it used. To get
the Python environment ready you can run `pipenv install` in shell at the
project root:

```shell
pipenv install
```

# wasm2native

A WebAssembly Binary to Native Code Compiler

## About the Project

I set the project to learn the details of building an industrial-level
compiler with LLVM technologies and modern compiler architecture brought
from the Swift compiler. The idea originated from the process that I
was trying to do some optimizations for the product that the company I'm
serving for on the level of the Swift compiler. I encountered a lot of
questions about how to use LLVM in this process. Thus I thought that the
best way to have answers to these questions is to build a compiler with
LLVM technologies and reuse the architecture of the Swift
compiler.

Even though this project started from my interest, it is open for
contribution.

WebAssembly is chosen because of its simplicity of grammar. This can
free developers of the project from complex frontend implementations and
enables them to focus on the middle-end and backend.

Moreover, WebAssembly + WASI is a good candidate for serverless and edge
computation. Investing in WebAssembly is not a bad choice at the current
moment.

## Build and Run the Code

> Apple Silicon Mac is the only supported host at the moment.

### Prerequisites

To build the project, you have to get CMake and ninja installed on your
computer.

For **macOS**:

Developers who use macOS may have [homebrew](https://brew.sh) installed on
their computer firstly.

```shell
brew install cmake ninja
```

### Getting Started

By running the following code you can get the monorepo setup and the
project built with debug configuration.

```shell
# make the monorepo directory
mkdir wasm2native-project

# change the directory to the monorepo directory
cd wasm2native-project

# clone the project with git
git clone https://github.com/WeZZard/wasm2native.git

# change the directory to the project directory
cd wasm2native

# clone the monorepo repositories
utils/update-checkout --clone

# build the project with debug configuration
utils/build
```

## Further Reading

- [Get the Develop Environment Ready](./docs/Get-the-Develop-Environment-Ready.md)
- [Understanding the Project Hierarchy](./docs/Understanding-the-Project-Hierarchy.md)
- [Understanding the Build Script](./docs/Understanding-the-Build-Script.md)
- [Understanding the Update Checkout Script](./docs/Understanding-the-Update-Checkout-Script.md)
- [Testing](./docs/Testing.md)
- [Debugging](./docs/Debugging.md)
- [Become a Contributor](./docs/Become-a-Contributor.md)

## License

Apache 2.0 with Runtime Library Exception

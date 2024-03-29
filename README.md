# wasm2native

A WebAssembly binary to native code compiler

## About the Project

I started the project to learn the details of building an industrial-level
compiler with LLVM technologies and modern compiler architecture brought
from the Swift compiler. The idea originated in the process when I
was trying to do some optimizations on the level of the Swift compiler for
the product of the company that I'm serving. Since I encountered a lot of
questions about how to use LLVM in the meanwhile, I brought out the idea
that the best way to have answers to those questions is to build a
compiler with LLVM technologies and reuse the architecture of the Swift
compiler.

But the project itself is open for contribution though it was started from
my interest.

WebAssembly is chosen because of its simplicity in grammar. This can
free developers of the project from complex frontend implementations and
enables them to focus on the middle-end and backend.

Moreover, WebAssembly + WASI is a good candidate for serverless and edge
computation. Investing in WebAssembly is not a bad choice at the current
moment.

## Status of the Project

### Roadmap

- [x] 1. Emit empty object file
- [x] 2. Emit object file with simple WebAssembly binary
- [x] 3. Introduce test-driven infrastructure for wasm-to-IR verification
- [ ] 4. Implement remaining register/stack/static-memory instructions
- [ ] 5. Implement dynamic memory runtime support and instructions
- [ ] 6. Implement the compiler driver
- [ ] 7. Implement import and export support

### Status of Components and Tools

Status Transition: None -> Prototype -> Ported from Swift -> WIP -> Done

| Components   | Status            | Tools  | Status    |
|:------------:|:-----------------:|:------:|:---------:|
| ABI          | Ported from Swift | Driver | Prototype |
| AST          | WIP               |        |           |
| Basic        | WIP               |        |           |
| Driver       | Ported from Swift |        |           |
| DriverTool   | Ported from Swift |        |           |
| Frontend     | WIP               |        |           |
| FrontendTool | WIP               |        |           |
| IRGen        | WIP               |        |           |
| Localization | Ported from Swift |        |           |
| Options      | Ported from Swift |        |           |
| Parse        | WIP               |        |           |
| Runtime      | None              |        |           |
| Sema         | WIP               |        |           |
| TBDGen       | Ported from Swift |        |           |

## Build and Run the Code

> Apple Silicon Mac is the only supported host at the moment.

### Prerequisites

To build the project, you have to get CMake and Ninja installed on your
computer.

For **macOS**:

Developers who use macOS may have [Homebrew](https://brew.sh) installed on
their computers first.

```shell
brew install cmake ninja
```

### Getting Started

By running the following code you can get the mono-repo setup and the
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

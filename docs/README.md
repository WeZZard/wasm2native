# Documentations of Wasm-2-Native

## The Goal of The Project

An evolving code base that helps me to understand:

- How to translate stack machine IR which here means to be Web Assembly
  instruction into register machine IR which here means to be LLVM IR.

- How to build a minimal but complete compiler project which includes: the
  compiler frontend, the compiler driver, dependency analysis and so on.

## Major Executables

The project is now planned with three major executables:

### Frontend

The frontend is planned to support the following commands:

```shell
# Modes:

# emit object file
w2n-frontend -emit-object [FILEPATH]... [-target TARGET]

# emit LLVM IR
w2n-frontend -emit-ir

# emit optimized LLVM IR
w2n-frontend -emit-irgen

# emit LLVM bitcode
w2n-frontend -emit-bc

# emit ast?
w2n-frontend -emit-ast # optional

# TARGET: An LLVM triple like "aarch64-apple-macos13.0". Use current host
# triple is target is not specified.
```

### Batch Driver

```shell

# Modes:

# emit executable file
w2nc [-emit-executable] foo.wasm

# emit library
w2nc -emit-library foo.wasm

```

### Interactive Driver

```shell

# Modes:

# build w2n package
w2n build

# create a w2n package
w2n package

# run a program from a w2n package
w2n run

# run test cases in a w2n package
w2n test

# Experiment with wasm instructions interactively
w2n repl

```

## Project Structure

The project is structured like the Swift compiler or LLVM which the entire
project is split into several components of libraries or executables to help
people to make use of the project even if they only need a singular part.

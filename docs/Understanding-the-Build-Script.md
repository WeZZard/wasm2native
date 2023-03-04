# Understanding the Build Script

The build script `utils/build` is designed to build the project with
the following step:

1. Build the toolchain which used for building the reset products in the
monorepo.
2. Install the toolchain to the toolchain directory in the monorepo.
3. Build other host tools with the toolchain just have been built.
4. Install the host tools to the toolchain directory in the monorepo.
5. Build wasm2native dependencies like LLVM
6. Build wasm2native

You can select product to build with arguments.

```shell
# Build the toolchain only
utils/build --build--toolchain

# Build LLVM only
utils/build --build--llvm

# Build WebAssembly Binary Toolkit only
utils/build --build--wabt

# Build wasm2native only
utils/build --build--wasm2native
```

Or build several products at the same time.

```shell
# Build LLVM and wasm2native
utils/build --build--llvm --build--wasm2native
```

> Product dependency resolution is not implemented now. You have to
> manually sort the product's order with the dependency graph's
> topological order.

If you are not sure what the arguments would result in, you can append
`--dry-run` to make the build script just print the commands it runs.

```shell
# Build LLVM and wasm2native with dry-run
utils/build --build--llvm --build--wasm2native --dry-run
```

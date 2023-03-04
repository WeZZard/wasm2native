# Understanding the Project Hierarchy

## Understanding the Monorepo

wasm2native relies on external projects such as LLVM and WebAssembly
Binary Toolkit in developing, testing and building. Since each external
project has its own repository organization pattern, wasm2native assumes
there is an umbrella directory as the monorepo directory to store the
wasm2native project itself and the repositories of external projects.

The monorepo directory initially should looks like what the following
diagram shows:

```ascii
-+ wasm2native-project           the monorepo directory
 |
 +- wasm2native                  the project directory
```

By running `utils/update-checkout` script with `--clone` argument, the
monorepo directory then have external repositories get cloned. There is a
diagram that shows what the monorepo directory would be after executing
`update-checkout --clone`:

```ascii
-+ wasm2native-project           the monorepo directory
 |
 +- wasm2native                  the project directory
 |
 +- [+] llvm-project             the llvm monorepo directory
 |
 +- [+] wabt                     the WebAssembly Binary Toolkit directory
```

> There may be components added or removed in the future.

By running `utils/build` script, we can get two more directories in the
monorepo:

```ascii
-+ wasm2native-project           the monorepo directory.
 |
 +- wasm2native                  the project directory.
 |
 +- llvm-project                 the llvm monorepo directory
 |
 +- wabt                         the WebAssembly Binary Toolkit directory.
 |
 +- [+] toolchain
 |
 +- [+] build
```

The two newly created directories are:

- `toolchain` directory: the installation directory for both the toolchain
used for building the product and the host tools used for testing and
developing the project.

- `build` directory: the directory stores built products.

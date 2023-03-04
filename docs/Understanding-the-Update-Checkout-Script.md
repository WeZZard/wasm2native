# Understanding the Update Checkout Script

The `utils/update-checkout` script manages the monorepo's repositories
checkouts with an array called `SCHEMA`:

```shell
SCHEMA=(
  'w2n wasm2native git ./ branch=main'
  'LLVM llvm-project git https://github.com/WeZZard/llvm-project.git branch=wasm2native/main'
  'WebAssemblyBinaryTool wabt git https://github.com/WebAssembly/wabt.git tag=1.0.32'
)
```

Each line in the `SCHEMA` array represents a repository that used by
wasm2native or just wasm2native itself. The information of the repository
is composed with a whitespace-seperated list which contains:

```shell
repo-name repo-directory-in-monodepo vcs-schema vcs-URL vcs-checkout
```

For example, the element `'w2n wasm2native git ./ branch=main'` means a
repository that:

- named `w2n`;
- claims the `wasm2native` sub-directory in the monorepo directory;
- uses `git` as the version control system;
- the `git` url is `./` (the current directory);
- uses the `main` branch checkout.

To **use a custom checkout** in a specific branch, you shall modify the
`vcs-checkout` for the repository entry in the `SCHEMA` array.

To **add a repository** to the monorepo, you shall add an entry in the
`SCHEMA` array by following the pattern mentioned above.

To **remove a repository** from the monorepo, you shall remove the
relative entry in the `SCHEMA` array.

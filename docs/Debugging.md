# Debugging

## Debugging Products

With Visual Studio Code configured with what mentioned in
[Get the Develop Environment Ready](./Get-the-Develop-Environment-Ready.md),
you can debug products in wasm2native by adding configurations in
`launch.json` with code like the following sample:

```json
{
  "configurations": [
    {
      "type": "lldb",
      "request": "launch",
      "name": "[DA] w2n-frontend",
      "program": "${workspaceFolder}/../build/${GENERATOR}-${CONFIG}/wasm2native-${OS}-${ARCH}/bin/w2n-frontend",
      "args": [
        // ... arguments of w2n-frontend
      ],
      "cwd": "${workspaceFolder}",
      "terminal": "console",
    }
  ]
}
```

## Debugging Unit Test Cases

With Visual Studio Code, debugging unit test cases is quite easy. You can
just click the "debug test" button on the right side of each case, then
Visual Studio Code would launch a debug process of the unit test cases.

## Debugging Regression Test Cases

// TODO: TBD

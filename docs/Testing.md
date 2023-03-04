# Testing

## Unit Tests

Unit tests are stored in the `unittests` directory.

With Visual Studio Code configured with what mentioned in
[Get the Develop Environment Ready](./Get-the-Develop-Environment-Ready.md),
Unit test cases shall be listed in Visual Studio Code's Testing panel.

## Regression Tests

Regression tests are stored in the `tests` directory and driven by LLVM
Integrated Tester (aka lit).

## LLVM Integrated Tester

LLVM Integrated Tester currently drives regression tests. In the future,
unit tests and validation tests would be driven by lit as well.

To run lit, you should install the lit program first. This can be done by
getting the Python environment ready. You can check this out in the
related section of
[Get the Develop Environment Ready](./Get-the-Develop-Environment-Ready.md).

If you are using Visual Studio Code and you want to run regression tests
in the edtior's terminal, then you have to choose Python interpreter of
the editor to the one in the virtual environment managed by the `pipenv`
which mentioned in
[Get the Develop Environment Ready](./Get-the-Develop-Environment-Ready.md).
After the Python interpreter changed, you can run regression tests with
the following command in Visual Studio Code's terminal.

```shell
lit ../build/${GENERATOR}-${CONFIG}/wasm2native-${OS}-${ARCH}/tests-${OS}-${ARCH}
```

If you don't want to run regression tests in Visual Studio Code's
terminal, you can run it with the following commands in your terminal:

```shell
pipenv shell
lit ../build/${GENERATOR}-${CONFIG}/wasm2native-${OS}-${ARCH}/tests-${OS}-${ARCH}
```

or just:

```shell
pipenv run lit ../build/${GENERATOR}-${CONFIG}/wasm2native-${OS}-${ARCH}/tests-${OS}-${ARCH}
```

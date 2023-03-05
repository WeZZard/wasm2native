# Become a Contributor

If you are looking for materials that can help you get your develop
environment ready, you should check out
[Get the Develop Environment Ready](./Get-the-Develop-Environment-Ready.md).

## Honor the Code Style

The project uses LLVM Code Style with several modifications:

1. Prefer `Foo * foo` instead of `Foo* foo` and `Foo *foo`.

The computer display goes larger and larger. The resolution of display
goes denser and denser. A developer may have its font size been set too
small to tell the difference between:

- `Foo foo` and `Foo *foo`;
- `Foo foo` and `Foo* foo`;

since the pointer qualifier `*` usually maps to an x-height glyph in most
fonts. This coulld introduce bugs which originally couldn't be.

![x-height](https://lh3.googleusercontent.com/iF6wBmzTnSHNp-T_P5fIOhLoqiaiPQGSkXwprlQYa7sYzxgieax7zCm2OgKXInHSsDIbgjUWZKUk2eM6y1yb7ud623QmleY6qfJdz4M=w1064-v0 "An illustration of x-height with alphabets xyMd")

The style `Foo * foo` makes an emphasis on the pointer qualifier. This
makes people to tell the difference between `Foo foo` and `Foo * foo` in
small font size or not well-designed fonts.

2. Prefer `Foo& foo` instead of `Foo &foo` and `Foo & foo`.

Since `&` is a part of `Foo&` which means "a reference to `Foo`" and not
a part of `foo` which is an instance of the reference, we don't prefer
`Foo &foo`. We also don't prefer `Foo & foo` here because `&` usually
maps to a glyph with cap-height.

![cap-height](https://lh3.googleusercontent.com/GV5FQ8hady9sQU0eHxUw_6O3TqPBxd1hezBNMSyw8WfdibMPZIMqt3x4gXVJWN7exKc-MT6teHqKNGnrbXPvLYq01weNCr2NVhVb5Q=w1064-v0 "An illustration of cap-height with alphabets xyMd")

A glyph with such a height already can make people to tell the difference
between:

- `Foo& foo` and `Foo foo`;
- `Foo& foo` and `Foo * foo`;

3. Use `<>` to quote public headers come from the projects in the
monorepo:

```cpp
#include <llvm/Support/Debug.h>
#include <w2n/Basic/SourceManager.h>
```

4. Use `""` to quote headers in `lib` directory.

```cpp
#include "IRGenInternal.h"
```

## Before Commit

1. Format the Code

If you have followed the steps in
[Get the Develop Environment Ready](./Get-the-Develop-Environment-Ready.md),
your code would be formatted when you saved files.

If you use other configures of develop environment you can format the code
in the project by executing the `format-code` script in the `utils`
directory:

```shll
utils/format-code
```

2. Diagnose the Code with clang-tidy

If you have followed the steps in
[Get the Develop Environment Ready](./Get-the-Develop-Environment-Ready.md),
clangd would diagnose your code when you are writing them.

If you use other configures of develop environment you can format the code
in the project by executing the `tidy-code` script in the `utils`
directory:

```shll
utils/tidy-code
```

This takes times to complete.

3. Commit message

- The commit message should start with a bracketed small letter phrase
indicates the contents you changed or "NFC". "NFC" means non-functional
change.

Small letter is not a requirement unless the phrase is an abbreviation.
This is because commiting is often the last step of a developer's coding
process. It should not take much energy to press the `shift` key to
compose a commit message. You should focus on the clearity and completness
of the message.

```ascii
// bad
Improve ir generation speed.

// good
[irgen] Improve ir generation speed.
```

- The commit message should be a complete sentence.

```asci
// bad
[frontend] Bugfix

// good
[frontend] Fixes frontend does not receive -emit-ir argument.
```

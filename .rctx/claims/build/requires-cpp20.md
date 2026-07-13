---
volatility: "stable"
watches:
  - "makefile"
reverify: "grep -q 'c++20' makefile"
---

The project requires a **C++20** toolchain. The `makefile` compiles with
`-std=c++20`, and the code depends on C++20 features (e.g. `std::format` in the
utils headers).

**Why it matters:** this is a hard floor, not a preference. An older compiler
default (`-std=c++17` or a system `g++`/`clang++` that predates C++20) will fail
to compile with errors that don't obviously point at the standard version. A
contributor cloning the repo has no in-code signal of the minimum standard.

**Note:** the CI workflow builds via `make` on `macos-latest` after
`brew install gcc`, so the effective compiler is Homebrew GCC. The `makefile`
`CXX = g++` therefore resolves to that GCC, not Apple Clang.

**If this changes:** bumping the standard (e.g. to C++23) will trip this claim's
drift check — update the `reverify` grep and this prose to the new floor.

---
volatility: "stable"
watches:
  - "CMakeLists.txt"
reverify: "grep -q 'CMAKE_CXX_STANDARD 20' CMakeLists.txt"
---

The project requires a **C++20** toolchain. `CMakeLists.txt` sets
`CMAKE_CXX_STANDARD 20` with `CMAKE_CXX_STANDARD_REQUIRED ON` and
`CMAKE_CXX_EXTENSIONS OFF`, and the code depends on C++20 features.

**Why it matters:** this is a hard floor, not a preference. An older compiler
or a lower standard will fail to compile with errors that don't obviously point
at the standard version. A contributor cloning the repo has no in-code signal of
the minimum standard.

**Build note:** the build is CMake + vcpkg (see ADR 0002). Locally the compiler
resolves to AppleClang; CI builds under its own toolchain. Either way the
standard floor is enforced by CMake, not by a compiler flag in a makefile — the
old `makefile` (which set `-std=c++20`) has been retired.

**If this changes:** bumping the standard (e.g. to C++23) will trip this claim's
drift check — update the `reverify` grep and this prose to the new floor.

---
volatility: "stable"
watches:
  - "vcpkg.json"
reverify: "test -f vcpkg.json && grep -q '\"builtin-baseline\"' vcpkg.json"
---

All third-party C++ dependencies come from **vcpkg in manifest mode**, declared
in `vcpkg.json` with a pinned `builtin-baseline` for reproducibility (see
ADR 0002). There are **no vendored third-party source trees** in the repo, and
includes are system-style (`#include <rapidjson/...>`, `<argparse/...>`).

Current declared deps: `rapidjson`, `argparse`, `tabulate` (table renderer),
and `doctest` (test-only).

**Why it matters:** a contributor must configure with the vcpkg toolchain
(`-DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake`, wired via
`CMakePresets.json`). Without `VCPKG_ROOT` set, `find_package(RapidJSON ...)` /
`find_package(argparse ...)` fail at configure time. A pinned baseline means
builds are reproducible; dropping it reintroduces version drift.

**If this changes:** if a dependency is added/removed, update the declared-deps
list above. If the baseline is intentionally advanced, note it here. Never
re-vendor a dependency to "fix" a build — fix the manifest or the toolchain.

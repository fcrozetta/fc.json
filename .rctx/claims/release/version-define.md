---
volatility: "stable"
watches:
  - "src/main.cpp"
  - "CMakeLists.txt"
reverify: "grep -q 'FC_VERSION' src/main.cpp && grep -q 'FC_VERSION' CMakeLists.txt"
---

The version reported by `fc-json --version` is baked in at **build time** via the
preprocessor define `FC_VERSION`.

- `CMakeLists.txt` declares `set(FC_VERSION "${PROJECT_VERSION}" CACHE STRING …)`
  and passes it as `target_compile_definitions(fc-json PRIVATE FC_VERSION="…")`.
- `src/main.cpp` consumes it: `argparse::ArgumentParser parser("fc-json", FC_VERSION)`,
  with a `#ifndef FC_VERSION` fallback of `"0.0.0-dev"` so a bare compile still works.
- Release CI (`.github/workflows/release.yml`) overrides it with the pushed git
  tag: `cmake … -DFC_VERSION="${GITHUB_REF_NAME}"`, then **verifies** the built
  binary reports exactly that tag before publishing.

**Why it matters:** this replaced an older, fragile mechanism where CI `sed`-ed a
literal `{{VERSION}}` token in the source. That could silently ship the literal
string if the token was moved or removed, with no test to catch it. The current
mechanism has a build-time default *and* a CI verify step, so a mismatch fails
the release instead of shipping a wrong version.

**If this changes:** if versioning moves to a generated header or is read at
runtime (e.g. from git), update the mechanism description above and repoint
`reverify`. Keep the CI verify-after-build step — it is the safety net.

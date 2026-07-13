# ADR 0002 — CMake + vcpkg as the build and dependency system

- **Status:** Accepted
- **Date:** 2026-07-13
- **Deciders:** Fernando Crozetta

## Context

The original build had three problems:

1. **Vendored dependencies, duplicated.** `rapidjson`, `argparse`, and
   `tabulate` are committed as source trees — and `rapidjson`/`argparse` exist
   in *two* copies (repo root and `fc.json/include/`). No version is pinned;
   updates are manual copy-paste; the tree is noise in every diff and clone.
2. **Two build definitions, neither authoritative.** An Xcode project (local
   dev) and a `makefile` (CI). The Xcode project is effectively the source of
   truth for local work, which is IDE-coupled and not reproducible from the CLI.
3. **Includes are path-coupled** to the vendored layout
   (`#include "include/rapidjson/document.h"`).

vcpkg is the author's default C++ dependency workflow. All three dependencies
are published in the vcpkg registry (`rapidjson`, `argparse`, `tabulate`).

## Decision

- **CMake is the single source of truth for the build.** The `makefile` is
  retired (or reduced to a thin convenience wrapper over CMake). The Xcode
  project, if kept, is generated via `cmake -G Xcode` — never hand-maintained as
  the authoritative build.
- **Dependencies come from vcpkg in manifest mode.** A committed `vcpkg.json`
  declares `rapidjson`, `argparse`, `tabulate`, with a pinned
  `builtin-baseline` for reproducibility. No third-party source lives in the
  repo.
- **The vendored trees are deleted**: `rapidjson/`, `argparse/`,
  `fc.json/include/`. Includes become system-style: `#include <rapidjson/...>`,
  `#include <argparse/argparse.hpp>`, `#include <tabulate/tabulate.hpp>`.
- **CI builds via CMake + vcpkg**, not `brew install gcc` + `make`. The
  toolchain file (`$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake`) is passed to
  CMake; `vcpkg` is bootstrapped in the workflow (or via the vcpkg GitHub
  action). The macOS sign/notarize/release steps are preserved.

## Consequences

**Positive**
- Reproducible, versioned dependencies; clean diffs and clones.
- One build definition, runnable from the CLI and CI identically. Matches the
  `uv`/`ruff` reproducibility discipline of the author's Python stack.
- IDE-agnostic; Xcode becomes a generated convenience, not a dependency.

**Negative / costs**
- vcpkg must be bootstrapped locally and in CI (`VCPKG_ROOT`), which is currently
  unset on the dev machine — a one-time setup cost.
- First CI build pays vcpkg dependency build time; mitigate with binary caching
  (GitHub Actions cache / `actions/cache` on the vcpkg archives).
- The `{{VERSION}}`↔`sed` release coupling (see claim
  `release/version-placeholder`) must be re-verified against the new CMake build
  path; version injection may move to a CMake `-D` define or generated header as
  a follow-up.

## Follow-ups

- Add rctx claim `build/deps-via-vcpkg` (watch `vcpkg.json`) once the manifest
  exists.
- Decide whether to keep a generated Xcode project or drop it entirely.
- Reconsider moving version injection from `sed` to a CMake define (would retire
  or rewrite the `release/version-placeholder` claim).

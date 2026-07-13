# C++ Conventions — fc.json

The standard this repo is held to. Derived from the author's Python discipline
(type-driven, layered, tooled) applied to C++. When in doubt, prefer
**explicitness and clarity over cleverness**.

These are rules, not suggestions. Deviations need a reason in the PR.

---

## Naming

- **One method-naming convention: `camelCase`.** No mixing `snake_case`
  (`read_json`) with `camelCase` (`addAction`) in the same codebase.
- **Types**: `PascalCase`, with the `FC` prefix for domain types (`FCItem`,
  `FCJson`). Keep the prefix — it's deliberate and recognizable.
- **Members**: `camelCase`, no Hungarian/`m_` prefix.
- **Constants / enum bridges**: reserve `kName` (Google-style) only for genuine
  compile-time constant tables; don't sprinkle it.

## Types — model with the type system, not with strings

- The domain model is **strongly typed**, the way a Pydantic model is. Never
  degrade a structured model to `map<string,string>` or parallel string arrays.
- Use `enum class`, never bare `enum`. No implicit int conversions.
- Prefer `std::optional<T>` over sentinel values; `std::variant<...>` over a
  "kind" enum plus a bag of maybe-valid fields.
- One field, one meaning. Don't carry two overlapping type discriminators on the
  same node.

## Ownership & memory

- **Be explicit about ownership.** Every pointer has a documented owner.
- For a tree/graph that gets **transformed** (nodes pruned, rewritten, merged),
  hold children by **index/ID into an arena** (`std::vector<Node>` + `size_t`),
  not raw pointers. Indices survive copies, container swaps, and prune passes;
  raw pointers into a `std::list` do not survive any of those.
- If a type owns resources or holds internal pointers, obey the **rule of 5** —
  or `= delete` copy/move explicitly. Never leave an implicit copy that would
  corrupt internal references.
- Reach for smart pointers (`unique_ptr`/`shared_ptr`) only when arena+index
  doesn't fit; justify `shared_ptr`.

## Null checks

- `nullptr` only. **Never `NULL` or `0`.**
- Prefer `if (ptr)` / `if (!ptr)` over `== nullptr` comparisons.

## Error handling

- **Runtime input errors → exceptions** (`throw std::runtime_error` or a typed
  exception). Report at the CLI boundary: message to `stderr`, non-zero exit.
- **`assert` is for programmer invariants only** — things that cannot happen if
  the code is correct. Never validate untrusted/runtime input with `assert`; it
  compiles out in release builds and your validation vanishes exactly where you
  ship.
- One error strategy per layer. Don't mix `assert`, `exit()`, and `throw` for
  the same class of failure.

## Headers

- **Never `using namespace` at file scope in a header.** It leaks into every
  translation unit that includes it. Qualify (`std::string`) or use a narrow
  `using std::string;` inside a function/namespace.
- `#pragma once` (this toolchain supports it) over hand-written include guards.
- Keep vendored third-party code (`rapidjson`, `argparse`, `tabulate`) behind
  the include boundary; do not let vendor types leak into domain headers.

## Const-correctness & construction

- Initialize members in the **constructor initializer list**, not by assignment
  in the body.
- Pass read-only parameters as `const T&` (or `T` + `std::move` when taking
  ownership). Don't take non-const `T&` unless you mutate the caller's object.
- Avoid silent copies: don't take `std::string` by value for a param you only
  read.
- Mark methods `const` when they don't mutate.

## Architecture boundaries (parse → IR → render)

- Three stages, cleanly separated:
  1. **Parse adapter** — the *only* code that knows `rapidjson` exists. Turns
     bytes into the IR.
  2. **IR** — the domain model. Vendor-agnostic. No `rapidjson::Type`, no
     `rapidjson::Value` in the IR's public surface.
  3. **Renderers** — pure functions/visitors over the IR (Pydantic, JSON-Schema,
     redacted example, table). Adding an output format must **not** require
     editing the IR node. This is the `render(model) -> str` split.
- Rendering logic does not live as methods on the data node. A `switch` in
  `getSchemaName()` that grows a case per output format is an anti-pattern here.

## Tooling (match the Python toolchain's discipline)

- **Build**: CMake is the source of truth. The Xcode project is a local-dev
  convenience, not the authoritative build. CI builds via CMake/make.
- **Format**: `.clang-format` committed; code conforms. Enforced in CI
  (`clang-format --dry-run --Werror`).
- **Lint**: `.clang-tidy` committed; run in CI.
- **Tests**: a real framework (Catch2 or doctest). New behavior ships with a
  test. Mirrors the "boot + live check" habit — a change isn't done until it's
  been exercised.

## Comments — BetterComments style

Annotate comments with a leading token so intent is visible at a glance (and
colorized by the BetterComments editor extension). Keep the honest
self-annotation habit — just tag it.

| Token | Meaning | Example |
|---|---|---|
| `// * ` | Highlight / important context | `// * Arena index, not a pointer — survives prune` |
| `// ! ` | Warning / caveat / gotcha | `// ! assert() compiles out in release; validate above` |
| `// ? ` | Open question / doubt | `// ? Why does rapidjson need Accept() here` |
| `// TODO: ` | Actionable follow-up | `// TODO: integers should map to int, not float` |
| `//// ` | Commented-out code (strikethrough) | `//// legacy map<string,string> path` |

Rules:

- Your existing doubt markers become `// ? ` — keep writing them.
- A `// ! ` or `// TODO: ` that guards a **known-wrong output** (e.g. "all
  numbers become float") is a stopgap, not a home. Promote it to a GitHub issue
  or an rctx claim; don't let it live in code indefinitely.
- `//// ` commented-out code is for a deliberate, short-lived reference during a
  change. Dead code that outlives the PR gets deleted, not strikethrough-parked.

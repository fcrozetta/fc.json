# ADR 0001 — Intermediate Representation and renderer split

- **Status:** Accepted
- **Date:** 2026-07-13
- **Deciders:** Fernando Crozetta

## Context

The original `fc.json` had no real intermediate representation. `FCItem`
conflated three concerns:

1. the parsed JSON structure,
2. the vendor parser's view of it (`rapidjson::Type rapidJsonType` stored on the
   node — the domain model was welded to rapidjson), and
3. rendering logic (`getSchemaName()` was a Pydantic renderer implemented as a
   `switch` living *on the data node*).

Consequences of that design, all observed in the code:

- Adding any new output (JSON-Schema, redacted example, table, diff) meant
  editing the node's `switch` — every format fought over the same method.
- The IR could not exist without rapidjson; it was untestable in isolation.
- Numbers all collapsed to `float` (integer information lost at no clear layer).
- Object schema names for array elements were path-derived
  (`Root/d/4` → `Root_d_4Schema`): ugly and collision-prone.
- Invalid Python identifiers (keys with spaces, keywords like `null`) passed
  through verbatim, emitting uncompilable Pydantic.
- A top-level non-object aborted via `assert(doc.IsObject())` (and asserts
  compile out in release → undefined behavior on that input).

The missing piece was a proper **Intermediate Representation (IR)**: a
vendor-agnostic, render-agnostic data structure that parsing fills and renderers
read.

## Decision

### Two IRs

- **`ValueIR`** — a faithful tree of the input, values retained. Produced by the
  parse adapter. Consumed by *value* renderers (pretty, redacted example, table,
  diff) and by transforms (redact/prune).
- **`SchemaIR`** — a merged, typed schema derived from `ValueIR` by a single
  `inferSchema` pass. Consumed by *schema* renderers (Pydantic, JSON-Schema).

Rationale: schema-merge logic (collapsing arrays of objects, optionality, union
handling) lives in exactly **one** place. A single faithful IR would force each
schema renderer to re-derive merge semantics and they would drift apart.

### Representation — arena, indices, no pointers

```cpp
using NodeId = std::uint32_t;                 // index into the arena
enum class NodeKind { Null, Bool, Int, Float, String, Array, Object };

struct Node {
    NodeKind kind;
    std::string key;                          // field name if child-of-object, else ""
    std::variant<std::monostate,bool,std::int64_t,double,std::string> scalar;
    std::vector<NodeId> children;             // object fields or array elements
};

class Tree { std::vector<Node> nodes; NodeId root; };
```

- Children are held by **index into an arena**, not raw pointers. This survives
  copies, container growth, and prune/transform passes — the ownership hazard of
  the old raw-pointers-into-`std::list` design is eliminated.
- **No parent pointers.** Paths are computed by passing a prefix down during
  traversal.
- **Int vs Float is decided at the parse boundary** (`IsInt64()` / `IsDouble()`),
  so the "everything is float" bug dies where rapidjson is quarantined.
- Scalars are retained so value renderers (example, redact, diff) have real data.

### Boundaries — parse → IR → render

1. **Parse adapter** is the only code that includes/knows rapidjson. It emits
   `ValueIR`. No `rapidjson::*` type appears in any IR or renderer signature.
2. **IR** is vendor-agnostic and contains no rendering logic.
3. **Renderers are pure free functions** over an IR, registered by output mode:
   `renderPydantic(const SchemaIR&)`, `renderJsonSchema(const SchemaIR&)`,
   `renderExample(const Tree&, RedactPolicy)`, etc. Adding a format never edits
   the IR.

### Schema semantics

- **Merge + optional.** Divergent object shapes within an array merge into one
  schema; fields absent from some instances become `Optional`. This matches
  hand-written Pydantic and is the most useful default. (Rejected: union of
  exact shapes — noisy; first-element-wins — silently wrong on real data.)
- **Identifier sanitization lives in the renderer, not the IR.** `SchemaIR`
  keeps raw JSON keys. The Pydantic renderer sanitizes to a valid Python
  identifier and emits `Field(alias="<original>")` to preserve round-trip.
  JSON-Schema keeps the key verbatim. Sanitization is a property of the target
  language, not the source.

### Object naming

- **Key-based, with structural dedup.** An object under key `address` →
  `Address`; two structurally identical objects → one class.
- **Array elements have no key.** An object that is an array element is named
  after the array's key, singularized: if the key ends in `s`, drop it
  (`users` → `User`); otherwise append `Item` (`a` → `AItem`). Deduped by shape.
- **Array of array of object**: only the leaf object needs a name (lists are
  anonymous); it takes the array key's singularized form.
- **Top-level array / scalar**: handled, not aborted. The root element schema
  uses a default name (`RootItem`), overridable later via a flag. This replaces
  the `assert(doc.IsObject())` crash.

## Consequences

**Positive**
- New output formats are additive: write a renderer, register it. No IR edits.
- IR is testable without rapidjson; the parser is the only thing that needs it.
- Int/float, identifier sanitization, and non-object roots are fixed by design,
  not patched.
- Transform passes (redact, prune) are safe against the arena model.

**Negative / costs**
- More types than the original single-class design (`ValueIR`, `SchemaIR`,
  `inferSchema`, per-format renderers).
- Structural dedup needs a stable structural hash of node shapes.
- Singularization is a naive heuristic; unusual plural keys yield awkward names
  (acceptable — deterministic and overridable).

## Follow-ups

- Renderer registry + CLI flag wiring (revive the stubbed `--pydantic`,
  `--schema`, `--redact`, `--table` flags against real renderers).
- Structural-hash design for dedup.
- Optional `--root-name` flag for top-level arrays.

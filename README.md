# fc-json

Inspect a JSON file and generate a schema, an example, or typed classes from its
structure. Point it at a `.json` file and pick an output format — Pydantic models,
JSON Schema, a redacted example, or a flat type table.

- [Install](#install)
- [Usage](#usage)
- [Output formats](#output-formats)
- [Build from source](#build-from-source)
- [License](#license)

## Install

Prebuilt binaries are published on the
[Releases page](https://github.com/fcrozetta/fc-json/releases) for macOS
(universal — Apple Silicon + Intel) and Linux (amd64 / arm64).

```bash
# Homebrew (macOS / Linux)
brew install fcrozetta/tools/fc-json
```

Or download the tarball for your platform from Releases and put `fc-json` on your
`PATH`. The binaries are **unsigned**, so on macOS a manually downloaded binary is
quarantined — clear it once:

```bash
xattr -d com.apple.quarantine ./fc-json
```

(Homebrew installs are not affected by this.)

## Usage

```bash
fc-json <input.json> [-f <format>]
```

- `input.json` — the JSON file to inspect (positional, required).
- `-f`, `--format` — one of `pydantic` (default), `json-schema`, `example`,
  `redact`, `table`.

Given a `sample.json`:

```json
{
  "a": { "b": "c" },
  "b": 1,
  "c": "asd",
  "d": [1, 2, 3, "asd", { "a": 123 }]
}
```

## Output formats

### `pydantic` (default)

```bash
fc-json sample.json
```

```python
from typing import Union
from pydantic import BaseModel

class A(BaseModel):
    b: str

class DItem(BaseModel):
    a: int

class Root(BaseModel):
    a: A
    b: int
    c: str
    d: list[Union[int, str, DItem]]
```

### `table` — a flat path → type view

```bash
fc-json -f table sample.json
```

```
+------+------------------------------+
| Path | Type                         |
+------+------------------------------+
| a    | A                            |
+------+------------------------------+
| a.b  | str                          |
+------+------------------------------+
| b    | int                          |
+------+------------------------------+
| c    | str                          |
+------+------------------------------+
| d    | list[Union[int, str, DItem]] |
+------+------------------------------+
```

### `json-schema`, `example`, `redact`

- `json-schema` — a JSON Schema (draft 2020-12) with `$defs` for nested objects.
- `example` — a skeleton JSON instance with placeholder values.
- `redact` — the input structure with scalar values redacted.

## Build from source

Requires CMake 3.21+, Ninja, and [vcpkg](https://github.com/microsoft/vcpkg).
Dependencies (rapidjson, argparse, tabulate, doctest) are resolved by vcpkg in
manifest mode against the pinned baseline in `vcpkg.json` — see
[ADR 0002](docs/adr/0002-cmake-vcpkg-build.md).

```bash
git clone https://github.com/fcrozetta/fc-json.git
cd fc-json
export VCPKG_ROOT=/path/to/vcpkg

cmake --preset default
cmake --build build
./build/fc-json --version
```

### Tests

`FC_JSON_BUILD_TESTS` is on by default, so the build above also builds the
doctest suite. Run it with:

```bash
ctest --test-dir build --output-on-failure
```

### Xcode

The Xcode project is generated on demand (it is not committed):

```bash
cmake --preset xcode
open build-xcode/fc-json.xcodeproj
```

## License

[GPL-3.0-or-later](LICENSE).

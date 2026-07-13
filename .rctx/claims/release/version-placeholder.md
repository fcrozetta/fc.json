---
volatility: "volatile"
watches:
  - "fc.json/main.cpp"
reverify: "grep -q '{{VERSION}}' fc.json/main.cpp"
---

`main.cpp` passes the literal token `{{VERSION}}` as the version string to the
argument parser (`argparse::ArgumentParser parser("fc.json","{{VERSION}}")`).

This is **not** a placeholder resolved at compile time. It is resolved by the
release CI (`.github/workflows/Build_and_Release.yaml`), which runs a `sed`
substitution replacing `{{VERSION}}` with the pushed git tag before building:

```
sed -i '' -e "s/{{VERSION}}/$TAG/g" fc.json/main.cpp
```

**Why it matters:** the coupling is invisible from the source alone. If anyone
removes, renames, or hard-codes over the `{{VERSION}}` token, the build still
compiles and CI still succeeds — but every released binary reports the literal
string `{{VERSION}}` for `--version`. There is no test that catches this.

**If this changes:** if version injection moves to a build-time define
(`-DFC_VERSION=...`), a generated header, or CMake, update or retire this claim
and point `reverify` at the new mechanism.

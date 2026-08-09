# Magma: A Ground-Truth Fuzzing Benchmark

The documentation has been moved to [the Magma homepage](https://hexhive.epfl.ch/magma).

## Custom Harnesses

This fork adds support for fuzzing with custom harnesses. Keeps harnesses in `targets/`
alongside other target scripts, then specify `<fuzzer>_HARNESSES` in your `captainrc`.

For example, `targets/libpng/custom` has two sample harnesses for AFL++.

> NOTE: Custom harness support is limited to C harnesses and AFL++.

Build with the following `captainrc`:
```bash
FUZZERS=(aflplusplus)
aflplusplus_TARGETS=(libpng)
aflplusplus_HARNESSES=(custom)
```

### Adding Custom Harnesses

1. Add harness files to `targets/<TARGET>`
1. Build harnesses in `targets/<TARGET>/build.sh`. See `targets/libpng/build.sh` for reference.

Target build scripts receive the `$HARNESS` environment variable, which is the value set in
`captainrc` with `<fuzzer>_HARNESSES=(...)`.

### Seeds

This fork adds support for _"default"_ seeds:
1. First check if `$TARGET/corpus/$PROGRAM` exists. If so, use that.
1. Otherwise use `$TARGET/corpus/default`.

This is so we don't have to duplicate seeds N times for all N harnesses we might generate.

# Custom Harness tracker:
|Library|Status|
|---|---|
|tinycbor| DONE|
|libpng|in-progress|
|sqlite3|DONE|
|libtiff|DONE|
|libaom|in-progress|
|...|...|

# TODOs
1) minor modification for harness building part of `libpng/build.sh`
1) libaom shows no instrumentation found

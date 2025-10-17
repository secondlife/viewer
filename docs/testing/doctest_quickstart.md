# doctest quickstart

This repository uses a header-only doctest setup with a thin helpers/compat layer.
- Enable: `autobuild configure -c RelWithDebInfoOS -- -DLL_TESTS=ON`
- Build targets: `llcommon_doctest`, `llmath_doctest`, `llcorehttp_doctest`
- Run: `ctest -C RelWithDebInfo -R "(llcommon_doctest|llmath_doctest|llcorehttp_doctest)" -V`

Notes:
- Hand-authored tests are marked with `// DOCTEST_SKIP_AUTOGEN` to keep the generator idempotent.
- `LL_CHECK_*` helpers provide clearer output for floats, buffers, wide strings, and ranges.
- HTTP fake layer provides zero-latency transport, monotonic clock, per-handle queued responses (redirects/retries/cancels) — tests stay network/IO-free.

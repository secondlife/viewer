# Doctest quickstart (Issue #4445)

The viewer is migrating unit tests from the unmaintained **tut** framework to
**[doctest](https://github.com/doctest/doctest)** (header-only, vendored under
`indra/extern/doctest/`).

## Status

- TUT remains the default for existing `*_test.cpp` targets (`LL_ADD_PROJECT_UNIT_TESTS` /
  `LL_ADD_INTEGRATION_TEST`).
- Parallel doctest suites live in per-library `tests_doctest/` directories and register
  with CTest via `add_test(...)`.
- First self-contained example: `indra/llmath/tests_doctest/llmodularmath_test_doctest.cpp`
  → CTest target `llmath_doctest`.

## Writing a doctest

```cpp
#include "linden_common.h"
#include "doctest.h"

TEST_SUITE("MyThing")
{
    TEST_CASE("does the thing")
    {
        CHECK(1 + 1 == 2);
        CHECK_MESSAGE(true, "human-readable failure context");
    }
}
```

Shared helpers:

- `indra/test/ll_doctest_helpers.h` — `LL_CHECK_MSG`, `LL_CHECK_APPROX`, range compares
- `indra/test/tut_compat_doctest.h` — optional TUT→doctest shims for generated ports
- `indra/test/doctest_main.cpp` + `lltest_harness.*` — APR / logging / LLTrace bootstrap

## Build & run (after a normal viewer configure with tests enabled)

```bash
# Configure with LL_TESTS=ON (viewer default when building tests)
cmake --build <build-dir> --target llmath_doctest
ctest --test-dir <build-dir> -R llmath_doctest --output-on-failure
```

## Migration tips

1. Prefer idiomatic `TEST_CASE` / `CHECK` over mechanical TUT shims for new ports.
2. Keep original TUT files until the doctest suite for that library is green in CI.
3. Then delete the TUT sources, drop `include(Tut)` / `use_prebuilt_binary(tut)`, and
   remove the `tut` package from `autobuild.xml`.
4. A mechanical helper lives at `tools/testing/gen_tut_to_doctest.py` (best-effort).

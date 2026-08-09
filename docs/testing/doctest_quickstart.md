# doctest quickstart

This repository uses a header-only doctest setup with a thin helpers/compat layer.

- Enable: `autobuild configure -c RelWithDebInfoOS -- -DLL_TESTS=ON`
- Build targets: module-local `*_doctest` targets, for example `llcommon_doctest`, `llmath_doctest`, `llcorehttp_doctest`, `llprimitive_doctest`, and `login_doctest`
- Run: `ctest -C RelWithDebInfo -R "doctest$" -V`

Notes:
- Hand-authored tests are marked with `// DOCTEST_SKIP_AUTOGEN` to keep the generator idempotent.
- `LL_CHECK_*` helpers provide clearer output for floats, buffers, wide strings, and ranges.
- The HTTP fakes layer keeps tests network/IO-free by simulating responses with a monotonic clock and per-handle queues (redirects, retries, cancels).

## Windows: local build with Autobuild and LL_TESTS

Assumes a Windows setup with Visual Studio and Autobuild available on PATH.

1. Open a “Developer Command Prompt for VS 2022” (x64).

2. Change to the viewer root, e.g.:

    cd D:\Projects\viewer

3. Ensure there is a `build-variables` directory next to the source tree.  
   If you already have `.build-variables` in this checkout, clone it locally:
    xcopy /E /I .build-variables build-variables

4. Configure a RelWithDebInfo build with tests enabled:
    autobuild configure -c RelWithDebInfoOS -- -DLL_TESTS=ON

5. Build the viewer and test binaries:
    autobuild build -c RelWithDebInfoOS

6. Run the doctest-based targets via CTest (adjust the build directory name if needed):
    ctest -C RelWithDebInfo ^
          -R "doctest$" ^
          -V --test-dir build-vc170-64

The Autobuild configuration name (`RelWithDebInfoOS`) maps to the Visual Studio configuration `RelWithDebInfo`, which is the value used with `ctest -C`.

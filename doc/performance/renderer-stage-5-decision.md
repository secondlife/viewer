# Renderer Stage 5 decision

## Decision

The controlled Apple Silicon native OpenGL baseline is blocked. Two full warm primes failed the committed schema-3 scene contract before any measured repeat began. Stage 5 retains no timing and makes no performance claim.

The first prime failed the placement, avatar, and assets gates. A second prime used the equivalent canonical Second Life URI to rule out the supplied web-map URL form. It failed placement, focus, avatar, and assets. The asset and self-avatar failures repeated while the viewer reported unsuccessful asset-cache writes and an incomplete self avatar.

The contract did its job. Loosening a gate after seeing these failures would turn a known invalid scene into a plausible-looking baseline.

## Build and test evidence

- Source was clean at `fa875954c55780153510975a271023ab0e66e782`, the committed Stage 4 contract.
- The checked-in `steady-warm-v1.json` manifest was not changed.
- All 34 Python benchmark tests passed.
- Both JSON schemas accepted all six manifests and the schema-3 fixture.
- The display and macOS directory integration tests passed.
- A clean benchmark-enabled universal ReleaseOS build compiled and packaged the Intel and Apple Silicon app.
- Both attempts used native OpenGL on the expected Apple GPU class.
- Power remained on AC with low-power mode off. The host reported no thermal or performance warning before or after the attempts.
- The dry run printed neither login identity nor destination.

The first packaging attempt exposed a stale generated `AUTOBUILD_EXECUTABLE` cache entry from an earlier disposable environment. Reconfiguring the ignored build tree with the current disposable Autobuild path fixed packaging. This did not change source or benchmark behavior.

## Sanitized rejection aggregate

| Gate | Rejected primes |
| --- | ---: |
| Placement | 2 |
| Focus | 1 |
| Avatar | 2 |
| Assets | 2 |

The runner produced zero measured artifacts. Rejected frame data, live logs, credentials, destination, isolated profiles, and visual material stay outside Git. The checked-in aggregate records gate names and counts only.

## Reanalysis

The five-repeat baseline cannot start from this account and isolated state. The repeated asset-cache write failures and incomplete self avatar make the asset gate the first dependency to investigate. Placement and focus also remain unresolved, but neither can make an asset-invalid prime usable.

The matched older-hardware control is no longer the next stage. It remains locked until one clean prime passes the existing contract. Renderer tracing, OpenGL changes, Vulkan work, threshold tuning, and hardware comparison also remain locked.

## Stage 6: isolated asset-readiness gate investigation

### Objective

Determine why a fresh isolated macOS prime cannot reach the existing all-zero asset rule or complete the self avatar. Do not collect or report performance timing.

### Scope

1. Reproduce one short non-timing launch from the committed schema-3 manifest and the same disposable profile rules.
2. Record privacy-safe facts for cache directory creation, VFS open and write status, pending asset requests, texture and mesh queues, and required self-avatar parts.
3. Distinguish a cache-path or purge-order failure from missing account wearables or server delivery failure.
4. Check whether the isolated warm cache survives long enough to serve a second validation launch.
5. Keep the placement, focus, avatar, and asset thresholds unchanged.
6. Do not alter the account outfit unless a separate operator decision authorizes that persistent change.

### Exit evidence

Stage 6 ends with one root-cause statement and one bounded correction or operator prerequisite. A validation prime must pass the unchanged assets and avatar gates before the baseline can return to the roadmap. Stage 6 must not execute the five measured repeats.

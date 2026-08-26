# Renderer Stage 7 decision

## Decision

The account and wearable attribution stage is complete, but the Apple Silicon
native OpenGL baseline remains blocked. Two full prime-only launches produced
the same privacy-safe classification:
`required-link-missing-or-unresolved`.

The account's Current Outfit Folder (COF) existed and was category-complete in
both launches. The viewer could not resolve a required shape, skin, hair, or
eyes link from that folder, and none of those four required wearables was
delivered to the avatar. Cache reuse passed. No measured repeat ran, no frame
timing was retained, and this stage makes no OpenGL, Vulkan, Zink, or hardware
performance claim.

## Contract delivered

- A benchmark-only viewer operation returns scene readiness and appearance
  facts in one main-thread response. The conservative avatar gate and its
  attribution therefore describe the same snapshot.
- The diagnostic path is requested only by prime-only readiness runs. Normal
  warm primes and measured runs retain the existing performance-data path.
- The appearance record contains fixed booleans for avatar validity, COF
  presence and completeness, COF-change context, four required-link states,
  four required-wearable delivery states, and final avatar readiness.
- A pure C++ classifier and a fail-closed Python projection enforce the fixed
  precedence and reject malformed, contradictory, or non-boolean facts.
- Appearance is optional in schema-3 observations. It changes no scene gate,
  threshold, policy hash, manifest hash, comparison field, or timing summary.
- Inventory identifiers, item names, account data, destinations, raw log text,
  and filesystem paths cannot enter the retained appearance record.

The fixed classifications are `avatar-unavailable`, `cof-incomplete`,
`required-link-missing-or-unresolved`,
`wearable-delivery-pending-or-failed`, `avatar-later-blocker`, and `ready`.
`unknown` is reserved for unavailable or invalid attribution.

## Verification

- All 57 Python benchmark tests passed.
- Draft 2020-12 validation passed for all six manifests, the schema-3 fixture,
  and a result containing the optional appearance record.
- The appearance, display, and cache-migration C++ integration tests passed
  with 6, 2, and 2 focused cases respectively.
- The benchmark-enabled universal Release build completed and contained both
  `arm64` and `x86_64` slices.
- Exactly two native Apple OpenGL prime-only launches ran on AC power with low
  power mode off and no reported thermal or performance warning.
- The viewer did not run after the sequence, and private credentials, logs,
  results, readiness reports, isolated profiles, and temporary state were
  removed.

## Sanitized two-prime evidence

| Fact | Prime 1 | Prime 2 |
| --- | --- | --- |
| Cache state | Writable and ready | Writable, sentinel reused, no fallback |
| Failed scene gates | Placement, avatar, assets, population | Placement, avatar, assets, population |
| Avatar object valid | Yes | Yes |
| COF present and complete | Yes | Yes |
| COF change in progress | Yes | Yes |
| Shape, skin, hair, and eyes links resolved | No for all four | No for all four |
| Shape, skin, hair, and eyes wearables delivered | No for all four | No for all four |
| Self avatar loaded | No | No |
| Appearance classification | `required-link-missing-or-unresolved` | `required-link-missing-or-unresolved` |
| Asset settlement window completed | Yes | Yes |
| Asset queues settled | No | No |
| Measurement | None | None |

The first prime observed no unresolved mesh work, while texture fetch and HTTP
work remained active. The second prime observed unresolved mesh work and more
texture activity. These are rejection facts, not comparable performance data.
They do not identify why the public scene remained unsettled.

## Reanalysis

The repeated account classification is stronger than the earlier log category.
It establishes that the COF itself was available and complete as an inventory
category, but the local inventory could not resolve any of its four required
body-part links during either full prime.

It does not establish whether each required link is absent from the COF or is
present with an unavailable target. Once a link target is unavailable locally,
the stable public inventory interface cannot reveal the target's intended
wearable type. The combined classification preserves that limit instead of
guessing. Likewise, four undelivered wearables are consistent with unresolved
links but do not prove a terminal server-side delivery failure.
`cof_change_in_progress` is supporting context, not causal proof.

The warm-cache lifecycle remains closed: the second launch reused the requested
writable asset cache and its sentinel without a known cache failure or fallback
asset root. Placement, population, and active asset work remain independent
scene blockers, but none can make the unresolved appearance state valid.

The original renderer-modernization plan therefore remains in master Stage 0.
Baseline timing, hardware comparison, tracing, OpenGL changes, Zink experiments,
renderer abstraction, and Vulkan implementation stay locked.

## Stage 8: establish a resolvable account outfit

### Objective

Satisfy or conclusively record the account prerequisite behind the Stage 7
classification. Verify that the COF contains resolvable links to shape, skin,
hair, and eyes before attempting another baseline. This is one committable
account-readiness stage inside master Stage 0. It collects no performance
timing and changes no renderer code or validity gate.

### Authorization boundary

Inspecting the account and rerunning the prime-only diagnostic are read-only.
Repairing the COF, wearing a replacement outfit, or changing any inventory link
is persistent account mutation and requires separate explicit authorization.
Without that authorization, record the operator prerequisite and stop before
changing the outfit or launching another validation sequence.

### Execution

1. Start from a clean checkout containing the Stage 7 commit. Re-run the focused
   tests only if the executable or environment has changed.
2. Using private runtime state, inspect the demo account's COF in the viewer and
   verify whether it visibly contains one link for each required body part. Do
   not retain item names, identifiers, screenshots, raw inventory output, or
   account details.
3. If a link is missing or broken, stop and report that exact prerequisite. With
   separate mutation authorization only, repair the four links or wear a known
   complete default outfit without changing graphics, destination, manifest, or
   benchmark policy.
4. After the operator verifies a resolvable COF, run exactly two prime-only warm
   launches against one disposable cache. Hold account, outfit, destination,
   manifest, build, and operator assertions fixed.
5. Require all four `required_links_resolved` facts to be true in both launches.
   Record whether all four required wearables are delivered, whether the avatar
   becomes loaded, cache reuse, and every unchanged failed scene gate.
6. Retain zero measured repeats and no timing. Remove credentials, logs, results,
   readiness output, isolated profiles, and temporary state.
7. Commit only the sanitized decision and any narrowly required diagnostic fix,
   reanalyze the next dependency, update the rolling plan, and stop.

### Exit gates

- Any account mutation was separately and explicitly authorized.
- The operator check chooses missing/broken COF links or confirms four resolvable
  required links without publishing private inventory data.
- A validation sequence, if eligible, uses exactly two fixed-state prime-only
  launches and the unchanged schema-3 gates.
- Cache reuse remains ready with no fallback or known write failure.
- Valid measured repeats remain zero and retained timing remains false.
- The repository and Mac checkout end clean and contain no private artifact.

If the links resolve but required wearables remain undelivered, the following
stage investigates delivery. If links and wearables are ready but the avatar is
not, it investigates the later appearance blocker. Only an avatar-ready,
asset-ready prime can unlock controlled-scene preparation and the five-repeat
baseline.

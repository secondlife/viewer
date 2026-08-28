# Embedded Browser

This document describes the embedded-browser system that replaces the legacy
CEF media plugin (`media_plugins/cef`, built on Dullahan and hosted by
`SLPlugin.exe`) for rendering web and media content inside the Viewer, both
in 2D UI floaters and on in-world prim faces.

## Why we needed this new approach

The legacy CEF media plugin runs each browser instance as a separate
`SLPlugin.exe` process, talking to the Viewer over the older `LLPlugin`
socket protocol that was originally designed as a generic plugin API, not
specifically for high-frequency pixel streaming. That protocol, and the way
each instance is a fully independent OS process, made it awkward to build on
for the things this project needed: a lighter-weight transport for streamed
video frames, a single host process that can hold many browser tabs at once,
and the ability to pick up an internally-built CEF distribution (with media
codec support enabled) without being tied to how Dullahan packages CEF for
the legacy plugin.

The embedded-browser system is a from-scratch CEF integration, built and
proven outside the Viewer first (as two standalone libraries and a testbed
application), then integrated into the Viewer. It uses shared memory instead
of the `LLPlugin` socket protocol to move rendered frames from a producer
process into the Viewer.

## What it is

Four pieces make up the system:

- **`llembeddedbrowser`** (`indra/llembeddedbrowser`) - lives inside the
  Viewer. This is the consumer: `LLEmbeddedBrowser` and
  `LLEmbeddedBrowserTab` own the shared-memory connection to the producer,
  forward input events, and hand back pixel frames and `EMediaEvent`-style
  notifications (load state, title, URL, cursor, and so on) to
  `LLViewerMediaImpl`/`LLMediaCtrl`.
- **`llshmframe`** (separate repo) - the shared-memory transport library.
  Defines the producer/consumer frame-publishing model and the heartbeat
  mechanism used to detect a dead or stalled producer.
- **`llcefbrowser`** (separate repo) - a CEF wrapper library. Owns the actual
  CEF browser instances, page navigation, input injection, and now audio
  muting (`SetAudioMuted`), independent of any Viewer-specific code.
- **`llcefproducer`** (`indra/llcefproducer`, builds to `SLCefProducer.exe`)
  - the producer. A single helper process, launched and stopped by the
  Viewer itself, that hosts every embedded-browser tab's CEF instance in one
  process and publishes rendered frames into shared memory for
  `llembeddedbrowser` to read.

`SLCefProducer.exe` is launched early in `LLAppViewer::init()` (gated on the
`UseEmbeddedBrowser` setting) and stopped again in `LLAppViewer::cleanup()`.
If it crashes or is killed mid-session, `llembeddedbrowser` detects the dead
heartbeat and relaunches it automatically (with a backoff limit, so two
Viewer instances racing for the same shared-memory channel do not loop
forever).

## Repo locations

- **`llcefbrowser`** - [`https://github.com/secondlife/llcefbrowser`](https://github.com/secondlife/llcefbrowser) (public)
- **`llshmframe`** - [`https://github.com/secondlife/llshmframe`](https://github.com/secondlife/llshmframe) (public)
- **`llcefshm-example`** - a personal testbed/example repo used during
  early development of `llcefbrowser`/`llshmframe` as standalone libraries,
  before either was integrated into the Viewer. Not part of the shipped
  product.
- **This Viewer repo** - [`https://github.com/secondlife/viewer`](https://github.com/secondlife/viewer),
  branch `callum/embedded-browser`. This work started in a separate private
  repo while the embedded-browser integration was still being proven out,
  then moved to a branch directly on the public Viewer repo once it was
  ready.

## CEF version used

The Viewer currently runs CEF `150.0.11+gb887805+chromium-150.0.7871.115`,
built internally (not from the public Spotify Automated Builds project)
with media codec support enabled, so formats like H.264 video play without
the "codec not found" errors a stock CEF build would show on sites like
Twitch. This is the same package `secondlife/dullahan` (the legacy plugin's
CEF wrapper) already consumes, uploaded once to Second Life's own S3 build
bucket since building it takes too long and too many resources to run in
GitHub Actions CI.

For a quick local build, `llcefbrowser`'s `tools/build.bat` points at this
same internal URL by default. A public Spotify CEF package will also build
and run, just without media codec support, useful for a contributor who
just wants a working local build and does not need to test media playback.

## Why shared memory, and not just linked straight into the Viewer

`llcefbrowser` hosts CEF's own multi-process architecture underneath it (a
browser/coordinator process plus a GPU process plus one renderer process
per view). Linking that directly into the Viewer's own process would put an
entire Chromium instance's stability and memory footprint inside
`secondlife-bin.exe` itself, so a CEF renderer crash could take the whole
Viewer down with it.

Instead, `SLCefProducer.exe` runs as its own separate OS process, and
rendered frames are handed to the Viewer through `llshmframe`'s shared
memory ring buffers rather than a socket. This is the same process
isolation principle the legacy `SLPlugin.exe`/Dullahan plugin already used;
the difference here is the transport (shared-memory frames, one producer
hosting many tabs) rather than the older `LLPlugin` socket protocol with one
process per instance.

## CEF process architecture: legacy versus embedded

The "one producer hosting many tabs" phrase above understates how different
the two backends' process trees actually are, confirmed by inspecting real
process command lines during a matched test (login, four preloaded UI
floaters, three prim media surfaces).

**Legacy (`media_plugin_cef`/Dullahan): one fully independent CEF stack per
media instance.** Each active media surface gets its own `SLPlugin.exe`,
launched separately via `LLPluginProcessParent` with no shared state with
any other instance. Each `SLPlugin.exe` hosts a complete Dullahan/CEF
instance internally, and CEF's own multi-process model re-execs that
instance's helper roles as a *different* binary, `dullahan_host.exe`: one
GPU process, one `network.mojom.NetworkService` utility process, one
`storage.mojom.StorageService` utility process, and one renderer process,
all per instance, none shared with any sibling instance. One active media
surface therefore costs up to 5 OS processes; N surfaces cost roughly 5xN.
In the test scenario above (8 active instances), this was 8 `SLPlugin.exe`
+ 40 `dullahan_host.exe` = 49 total processes.

**Embedded (`llembeddedbrowser`/`SLCefProducer`): one shared CEF browser
process family for the whole session.** `SLCefProducer.exe` hosts
`llCefBrowserManager`, which manages every tab (every UI floater and every
prim media surface) as separate CEF "browser" handles within *one shared
browser-process context*. CEF's multi-process model still applies, but the
shared layers are amortized across all tabs: one GPU process and one
utility-process pair (network + storage) total, regardless of tab count,
with only the renderer count scaling roughly per active tab (CEF's own
site-isolation still applies at that layer). Unlike legacy, these helper
roles re-exec under the *same* binary name, `SLCefProducer.exe` -- a single
process-name filter catches everything on this side, while legacy needs
both `SLPlugin` and `dullahan_host` (see `scripts/perf/memory_compare.ps1`,
which got this wrong at first for exactly this reason). Same 8-instance
scenario: 1 producer + 12 children (9 renderer, 1 GPU, 2 utility) = 13
total processes.

**The tradeoff.** Legacy's full per-instance isolation means one instance's
GPU/utility/browser-layer crash can't touch any other instance at all --
maximally isolated, but the most expensive model per additional tab.
Embedded's shared model is dramatically cheaper per additional tab
(confirmed: roughly 27% less WorkingSet in a matched test despite embedded
having more active tabs' worth of renderers than legacy had media
instances), at the cost of a shared GPU/utility layer -- a crash there is a
session-wide event rather than a single-tab one, though Chromium's
renderer-level crash isolation (one bad page does not take down the browser
process) still holds either way.

## Cookies: UI and prim media are isolated by default

Embedded-browser content uses two separate `CefRequestContext` instances:
one for the Viewer's own UI-hosted web content (the login floater, search,
marketplace, and so on), and a separate one for prim/in-world media. This
keeps cookies, including the resident's OpenID login cookie, from
automatically flowing into content authored by other residents.

A single switch controls whether the OpenID cookie is also shared with prim
media: `kShareOpenIDCookieWithPrimMedia`, a `static const bool` in
`indra/newview/llviewermedia.cpp`, inside `LLViewerMedia::getOpenIDCookieCoro()`
(currently around line 1468, clearly marked with a `POLICY SWITCH` comment
block above it). It defaults to `true`, matching the legacy Viewer's
behavior of sharing the login cookie with everything. Flip it to `false` and
rebuild for the more secure isolated default, where only UI-floater media
gets the logged-in resident's identity.

## Preloading pages

`LLViewerWindow::initWorldUI()` (`indra/newview/llviewerwindow.cpp`, right
after login) pre-creates four floaters that host embedded-browser content -
`destinations`, `avatar_welcome_pack`, `search`, and `marketplace` - by
calling `LLFloaterReg::getInstance()` on each. Constructing a floater starts
its embedded-browser tab navigating right away, in the background, so the
page is already loaded by the time the resident actually opens it instead of
starting cold.

This is gated on physical memory: it only runs on machines with more than 8
GB of RAM. On lower-end machines, only the welcome pack is preloaded, and
only on a resident's very first login; everything else waits until the
resident opens it themselves.

To add another floater to this preload set, add another
`LLFloaterReg::getInstance("floater_name")` call alongside the existing ones
in that same block.

## MediaMaxInstances: the hard cap on concurrent media

There are two separate limits on how many media instances can be active at
once, and they are easy to confuse:

- **`SLCefProducer.exe`'s own hard ceiling is 32 concurrent shared-memory
  channels** (`kSlotCount` in `cefshm_protocol.h`/`llcefproducer.cpp`). This
  is a structural limit on the transport itself, not something normally
  worth tuning.
- **`MediaMaxInstances`** (a saved setting, debug settings search "Media",
  default `12`) is the real, practically-relevant limit: `LLViewerMedia`'s
  own hard cap on how many media instances (UI floaters and in-world prim
  media together) it will actually keep loaded at once, comfortably inside
  the 32-slot ceiling above. On a machine with less than 8 GB of RAM this
  is reduced by 2.

This setting is **not embedded-browser-specific**, despite living in this
document - `LLViewerMedia::updateMedia()`'s priority/cap-accounting loop
iterates every media instance regardless of backend, so it counts
legacy-plugin and embedded-browser instances against the same combined
total. It was renamed from the older `PluginInstancesTotal` since that name
no longer described what it does (media plugins are not built or shipped by
default any more - see "Known limitations" below), but the cap itself
remains shared code, not new to this project.

The four preloaded floaters above always count against this cap once
constructed. With the default of 12, that leaves 8 slots free for
everything else (in-world prim media plus any other UI media a resident has
open) before the cap starts refusing new instances (they show as
unloaded/blank rather than failing outright).

One easy-to-misread symptom worth knowing about: a media instance a
resident has just walked away from does **not** free its cap slot
immediately. `mInterest` (the signal driving unload decisions) is a
"how large did this render recently" stat that fades gradually over several
seconds, deliberately, to avoid loading/unloading media on every small
movement. A resident who walks briskly past several media-bearing prims
toward more media can see the newer prims briefly lose the cap contest to
the ones they just left, for up to several seconds until the older
instances' interest actually decays out - a real but temporary effect, not
a bug, and not something a resident who deletes or stops actively passing
stale media will ever encounter.

## Distance/priority-based render throttling

The legacy CEF plugin throttled a media instance's own render rate and
resolution once it fell far enough away or out of interest
(`LLPluginClassMedia::setPriority()`/`setLowPrioritySizeLimit()`). Embedded
browser had no equivalent for a while: every CEF instance the producer held
rendered and published frames at full rate no matter how far away or
uninteresting it was, which wasted CPU on busy, media-heavy regions.

This is now fixed with a producer-side render throttle. `SLCefProducer.exe`
drives CEF manually via `SendExternalBeginFrame()`, called once per tab per
tick; a new wire command, `kSetRenderRate`, lets the Viewer cap how often
that call actually fires for a given tab (0 means unthrottled, the
default). `LLViewerMediaImpl::setPriority()` maps the same priority value
`LLViewerMedia::updateMedia()` already computes for every media instance
(distance, screen size, focus, CPU budget, and so on) to a target frame
rate for non-UI, non-parcel prim media only:

| Priority               | Target rate |
|-------------------------|-------------|
| `PRIORITY_NORMAL`/`HIGH` | unthrottled |
| `PRIORITY_LOW`           | 30 fps      |
| `PRIORITY_SLIDESHOW`     | 2 fps       |
| `PRIORITY_HIDDEN`        | 1 fps       |

These specific numbers (`EMBEDDED_BROWSER_FPS_LOW`/`SLIDESHOW`/`HIDDEN` in
`llviewermedia.cpp`) are a starting point for the product team to tune
further, not a final answer. `PRIORITY_LOW` in particular is reachable just
by losing focus, not only by real distance: `LLViewerMediaFocus`'s own
auto-zoom moves the camera in when a media face is clicked and back out
again when it's clicked off, so defocusing genuinely shrinks that media's
on-screen footprint (see the `media_is_small` heuristic in
`LLViewerMedia::updateMedia()`) even for a resident standing in the exact
same spot, still watching it. `PRIORITY_LOW`'s rate is set relatively high
(30fps, not a harsher number) specifically so that very common case doesn't
look like a jarring quality drop.

**Demotions are debounced, promotions are not.** A newly computed render
rate that is *worse* than the one currently applied only takes effect after
it holds for `EMBEDDED_BROWSER_RENDER_RATE_DEMOTION_GRACE_PERIOD` (2
seconds) - recovering to a better tier before that cancels the pending
demotion with nothing applied at all. This exists for the same click-off/
auto-zoom case above: instantly dropping frame rate the moment focus is
lost, before the resident has actually moved anywhere, is exactly the kind
of visible, unnecessary quality hit this whole project is trying to avoid.
A rate that gets *better* always applies immediately - there's no reason to
delay giving media its full rate back.

**UI and parcel media are structurally exempt, not just usually fine.**
This project's whole reason for existing is partly that the legacy CEF
plugin was widely felt to be slow and clumsy, and priority mis-assignment
was suspected as a possible cause (UI elements should always run at full
speed, no exceptions). The same shared priority computation above can push
even a UI floater down to `HIDDEN`/`LOW` when the Viewer window is
minimized or loses focus, so the throttle deliberately does not key off the
raw priority value alone. UI (`mUsedInUI`) and parcel media always send a
target rate of 0, unconditionally, regardless of what priority they're
assigned - the same population split already used elsewhere in
`setPriority()` for the auto-unload debounce. Only ordinary in-world prim
media, the same population the legacy plugin throttled, is ever affected.

## Shared-memory footprint and EmbeddedBrowserMaxWidth/EmbeddedBrowserMaxHeight

Each media tab's shared-memory segment is a lock-free triple buffer
(`llshmframe`'s own design - see its header comment), sized once, up front,
to whatever pixel ceiling that tab might ever be resized to. That ceiling
used to be a single hardcoded producer-side constant (1920x1080) applied to
every tab regardless of what the Viewer actually needed, which meant a
resident who lowered `EmbeddedBrowserMaxWidth`/`EmbeddedBrowserMaxHeight`
(already an existing setting, clamping what size the Viewer *requests*) got
no shared-memory savings at all - the producer still reserved the same
1920x1080x3-buffers segment either way, on every low-end machine and every
high-end one alike.

`kRequestSlot` now carries the Viewer's own current
`EmbeddedBrowserMaxWidth`/`Height` alongside the existing UI/prim flag, and
the producer sizes that specific tab's shared-memory segment to
`min(what the Viewer asked for, the producer's own absolute maximum)`
instead of always reserving the absolute maximum. Raising the setting above
the producer's own ceiling changes nothing (that ceiling still wins), but
lowering it now genuinely shrinks the memory each tab reserves, not just
what resolution it's allowed to request. This is a real, additional lever
for constrained machines, on top of (not a replacement for) the render-rate
throttling above - one shrinks how much memory a tab reserves, the other
shrinks how much CPU it costs to keep painting.

## Debugging

Two separate log files are written next to `SLCefProducer.exe`:

- **`cefshm_producer_log.txt`** - CEF's own internal log. Larger and much
  more verbose than the producer's own log; this is the one to check for a
  CEF-level crash or renderer problem.
- **`slcefproducer_log.txt`** - the producer's own console-mirroring log.
  The same lines you would see in a visible console window if
  `EmbeddedBrowserProducerConsole` is turned on, written to disk regardless
  of whether that console is showing.

A few debug settings (Advanced > Show Debug Settings, or Preferences, search
for "Embedded" or "Cef") control embedded-browser diagnostics. All of them
take effect on the next `SLCefProducer.exe` launch, not immediately:

- **`EmbeddedBrowserProducerConsole`** (boolean, default off) - if true,
  `SLCefProducer.exe` allocates a visible console window showing its
  diagnostic output.
- **`EmbeddedBrowserDebugging`** (boolean, default off) - enables
  `llembeddedbrowser`/`SLCefProducer` debugging features. Currently this
  just gates the remote-debugging port setting below.
- **`EmbeddedBrowserRemoteDebuggingPort`** (unsigned integer, default 0) -
  when non-zero and `EmbeddedBrowserDebugging` is also true,
  `SLCefProducer.exe` serves Chrome's remote-debugging-protocol DevTools UI
  on that port.

## Media Monitor: a live list of active media

`LLFloaterMediaMonitor` (`indra/newview/llfloatermediamonitor.{h,cpp}`,
XUI name `media_monitor`) is a debug/QA floater listing every currently
active embedded-browser media instance in one sortable table: slot,
priority, media type (UI, Prim, or Parcel), URL, and in-world location.
URL and Location cells are hot-linked - double-clicking a URL opens it in
the desktop browser, and double-clicking a Location teleports there. The
list refreshes automatically every two seconds while the floater is open,
and the title bar shows a live "N/max instances" count alongside it (`N`
active right now, `max` the current `MediaMaxInstances` cap) - a count at
or near the cap is an immediate, visible explanation for "why didn't this
render."

Open it from **Develop > UI > Media Monitor** (post-login) or **Debug >
Media Monitor** (login screen), or press `Ctrl+Alt+Shift+Y` in either
context - both menus are gated behind the `UseDebugMenus` setting, same as
every other item under them.

A couple of things worth knowing about what it shows:

- **URL clicks always open externally**, via
  `LLUrlAction::openURLExternal()`, deliberately bypassing the Viewer's
  normal internal-versus-external browser routing
  (`LLWeb::useExternalBrowser()`, keyed off the `PreferredBrowserBehavior`
  setting and each URL's domain) that every other link click in the Viewer
  goes through. For a QA tool, one URL always opening in one predictable
  window matters more than matching that routing.
- **Location is only ever populated for Prim media.** UI and Parcel media
  do not resolve to a single object a location can be derived from - a
  Parcel media row does have a real location (the parcel/region it is
  playing in) but not one this floater currently surfaces; it shows `-`
  there rather than a fabricated one.
- **The list reflects the Viewer's own bookkeeping, not a live producer
  query.** `LLViewerMedia::getEmbeddedBrowserDebugInfo()` reads
  `LLViewerMediaImpl`'s already-tracked state (the same priority list and
  per-instance fields `LLViewerMedia::updateMedia()` already maintains),
  not a round-trip to `SLCefProducer.exe`. This is sufficient for the
  "what's showing and where" QA use case, but would not by itself catch a
  producer-side desync (a slot the Viewer has lost track of) - a genuine
  future extension if that ever becomes a real debugging need.
- **The `max` in the title is the shared `MediaMaxInstances` cap, not an
  embedded-browser-specific one.** `LLViewerMedia`'s cap-accounting counts
  legacy-plugin and embedded-browser instances together against the same
  total (see "MediaMaxInstances" above), but this floater's `N` only
  counts embedded-browser rows. Not a live discrepancy today, since the
  legacy media plugin is not built by default in this branch, but worth
  knowing if that ever changes.

## Windows Firewall prompt on the first WebRTC connection

The first time any page loaded through the embedded browser negotiates a
WebRTC connection (voice or video chat, for example), Windows may show a
one-time "Windows Security - Do you want to allow public and private
networks to access this app?" prompt for `SLCefProducer.exe`. This is not
a code-signing problem and not related to `EmbeddedBrowserDebugging` or
the remote-debugging port below. It is a known, common Chromium/CEF-wide
behavior: WebRTC's local-IP-hiding privacy feature obfuscates a page's
real local IP address during ICE candidate gathering by minting a random
`.local` mDNS hostname and binding a UDP socket to answer queries for it,
and that bind is exactly what triggers this Windows Firewall dialog.

As of `llcefbrowser` 1.32.0, this is disabled (`--disable-features
WebRtcHideLocalIpsWithMdns` in `llCefBrowserLib.cpp`), so this prompt
should no longer appear. The tradeoff: real local IPs are exposed to the
remote peer during WebRTC ICE negotiation instead of mDNS-obfuscated, an
acceptable one for a windowless embedded browser with no user-facing
privacy UI of its own to explain the prompt otherwise.

Two earlier hypotheses were investigated and ruled out before finding the
real cause, in case this ever needs revisiting: `SLCefProducer.exe`'s code
signature (confirmed valid via `Get-AuthenticodeSignature`) and its
embedded manifest's execution level (confirmed `asInvoker`, no elevation
requested). Neither was the issue.

## Developer console

An earlier attempt used CEF's own in-process DevTools popup
(`CefBrowserHost::ShowDevTools`), which crashed for reasons never fully
confirmed even after several attempted fixes. Chrome's remote-debugging
protocol was used instead, and is more useful in practice anyway since it
works from any desktop browser, not just a popup window glued to the
Viewer.

To use it:

1. Turn on `EmbeddedBrowserDebugging`.
2. Set `EmbeddedBrowserRemoteDebuggingPort` to a non-zero port (for example,
   `9222`).
3. Relaunch the Viewer, so `SLCefProducer.exe` starts fresh with the new
   settings.
4. Open `http://localhost:<port>` (or `chrome://inspect` configured to that
   port) in any desktop Chromium-based browser. This lists every live
   embedded-browser tab, UI floater or prim media, and opens a full DevTools
   session against whichever one you pick.

For performance debugging, DevTools' FPS meter is useful here: with a
session open, press Ctrl+Shift+P (Command+Shift+P on macOS) to open the
Command Menu, type "Rendering", and select "Show Rendering". In the
Rendering panel that opens, check "Frame Rendering Stats". This overlays a
real-time FPS/dropped-frames/GPU-raster readout directly on the page,
useful for checking the distance/priority render-throttling tiers above
are actually taking effect on a given tab.

## Known limitations, as of this writing

- **Volume is all-or-nothing.** CEF only exposes a binary
  `SetAudioMuted()`/`IsAudioMuted()` per browser instance, not a continuous
  0-100% volume control, and `SLCefProducer.exe` hosts every tab in one
  shared OS process (confirmed via Windows' own Volume Mixer, which shows a
  single `SLCefProducer` session even with two tabs playing audio at the
  same time). A per-tab volume slider is not possible with this
  architecture today; each tab can only be fully muted or fully unmuted.
- **Media codec coverage versus LibVLC is still an open question.** The
  internally-built, codec-enabled CEF distribution plays a good range of
  formats, but not as many as LibVLC, which the legacy media plugin can
  fall back to. Whether CEF's coverage is sufficient for the vast majority
  of real-world content, making a LibVLC re-integration unnecessary, is
  under discussion with the product team and not yet decided.
- **Windows only, for now.** This first version of the embedded-browser
  system targets Windows exclusively. macOS and Linux support is planned to
  follow.
- **The legacy media plugin has not been removed.** `media_plugins/cef`,
  Dullahan, and `SLPlugin.exe` remain in the codebase alongside the
  embedded-browser system. Removing them now would not be a clean, isolated
  deletion (that plugin framework also backs non-CEF streaming-audio
  plugins, and a few files have CEF-specific calls not yet ported to the
  new backend), so the legacy code stays in place until the embedded-browser
  system is closer to feature-complete.

## Future work

- **GPU texture handles instead of CPU memory-buffer copies.** Today, every
  frame CEF renders is delivered via its `OnPaint` callback as a raw BGRA
  pixel buffer in CPU memory, which `SLCefProducer.exe` then copies into an
  `llshmframe` shared-memory ring buffer for the Viewer to read back out
  and upload to a GL texture, a full-frame CPU copy (and a GPU readback
  before that) on every update. CEF also exposes `OnAcceleratedPaint`,
  which instead hands back a GPU-side shared texture handle (a DXGI/D3D11
  shared resource on Windows) with no CPU copy involved at all. Switching
  to that path would let `llshmframe` pass a texture handle across the
  process boundary instead of raw pixels, but requires the Viewer to
  import that shared resource into its own D3D/GL context
  (`ID3D11Device::OpenSharedResource` or equivalent), a real transport
  redesign rather than a small change, not started.
- **Reintroduce LibVLC support, for parcel audio and additional video
  format support.** Likely essential before a first release, in the same
  category as macOS/Linux support above rather than a true nice-to-have.
  Parcel music streaming (`LLStreamingAudio_MediaPlugins`, see
  `indra/newview/llviewermedia_streamingaudio.h`) plays through the
  plugin architecture, and routes pure audio streams to
  `media_plugin_libvlc` specifically, not CEF. Since `ENABLE_MEDIA_PLUGINS`
  now defaults off (see "The legacy media plugin has not been removed"
  above), that plugin no longer builds or deploys, so parcel audio
  streaming does not currently work in this branch. This is separate
  from the "Media codec coverage versus LibVLC" question above, which is
  about video formats embedded in web pages, not parcel audio.

## Notes on AI-assisted development

This system was developed with substantial help from Claude (Anthropic's
Claude Sonnet 5), used as a collaborator rather than a code-generator to
point at a problem and merge unreviewed. Every change was reviewed, and most
were verified end-to-end against a real repro before and after, not just
compiled and assumed correct.

The development path itself went through several stages: `llcefbrowser` and
`llshmframe` started as external, standalone experiments outside the
Viewer entirely; those were used to build a standalone testbed application
(`llcefshm-example`) to prove the producer/consumer shared-memory model
worked on its own; only after that was proven out did the integration into
this Viewer, with Claude, begin.

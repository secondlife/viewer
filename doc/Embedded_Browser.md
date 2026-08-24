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

- **`llcefbrowser`** - `https://github.com/secondlife/llcefbrowser` (public)
- **`llshmframe`** - `https://github.com/secondlife/llshmframe` (public)
- **`llcefshm-example`** - a personal testbed/example repo used during
  early development of `llcefbrowser`/`llshmframe` as standalone libraries,
  before either was integrated into the Viewer. Not part of the shipped
  product.
- **This Viewer repo** - `https://github.com/secondlife/viewer-embedded-browser`,
  branch `callum/viewer-embedded-browser`. Private for now, while the
  embedded-browser integration itself is still being proven out; `llcefbrowser`
  and `llshmframe` are already public since they carry no Viewer-specific
  code.

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

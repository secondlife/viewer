<picture>
  <source media="(prefers-color-scheme: dark)" srcset="doc/sl-logo-dark.png">
  <source media="(prefers-color-scheme: light)" srcset="doc/sl-logo.png">
  <img alt="Second Life Logo" src="doc/sl-logo.png">
</picture>

**[Second Life][] is a free 3D virtual world where users can create, connect and chat with others from around the
world.** This repository contains the source code for the official client.

## Open Source

Second Life provides a huge variety of tools for expression, content creation, socialization and play. Its vibrancy is
only possible because of input and contributions from its residents. The client codebase has been open source since
2007 and is available under the LGPL license. The [Open Source Portal][] contains additional information about Linden
Lab's open source history and projects.

## Download

Most people use a pre-built viewer release to access Second Life. Windows and macOS builds are
[published on the official website][download]. More experimental viewers, such as release candidates and
project viewers, are detailed on the [Alternate Viewers page](https://releasenotes.secondlife.com/viewer.html).

### Third Party Viewers

Third party maintained forks, which include Linux compatible builds, are indexed in the [Third Party Viewer Directory][tpv].

## Build Instructions

### Initial Configuration

Configure the build for the first time by running:
```bash
cmake -P configure.cmake
```

This will create a platform-specific build directory:
- **Windows**: `build-vc170-64`
- **Linux**: `build-linux-x86_64`
- **macOS**: `build-darwin-x86_64`

### Reconfigure (if needed)

**Note**: The `configure.cmake` script handles most configuration scenarios automatically. Manual reconfiguration is typically only needed if you want to change specific CMake options or if there were issues during the initial configuration.

After the initial configuration, the build can be reconfigured by running:

- **Windows**: `cmake -B ./build-vc170-64 -S ./indra/`
- **Linux**: `cmake -B ./build-linux-x86_64 -S ./indra/`
- **macOS**: `cmake -B ./build-darwin-x86_64 -S ./indra/`

Additionally GUI based configuration can be used by replacing `cmake` with `cmake-gui` in the above commands.


### Build the Project

Build the project by running:

- **Windows:** `cmake --build build-vc170-64 --parallel`
- **Linux:** `cmake --build build-linux-x86_64 --parallel`
- **macOS:** `cmake --build build-darwin-x86_64 --parallel`

### Run Tests

Run the tests by running:

- **Windows:** `cmake --build build-vc170-64 --parallel -t tests_ok`
- **Linux:** `cmake --build build-linux-x86_64 --parallel -t tests_ok`
- **macOS:** `cmake --build build-darwin-x86_64 --parallel -t tests_ok`

## Contribute

Help make Second Life better! You can get involved with improvements by filing bugs, suggesting enhancements, submitting
pull requests and more. See the [CONTRIBUTING][] and the [open source portal][] for details.

[Second Life]: https://secondlife.com/
[download]: https://secondlife.com/support/downloads/
[tpv]: http://wiki.secondlife.com/wiki/Third_Party_Viewer_Directory
[open source portal]: http://wiki.secondlife.com/wiki/Open_Source_Portal
[contributing]: https://github.com/secondlife/viewer/blob/main/CONTRIBUTING.md

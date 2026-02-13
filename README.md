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

The Second Life viewer uses CMake for build system generation and vcpkg for dependency management. 

### Platform-specific setup guides

[Windows](doc/BUILD.WINDOWS.md)

[Mac](doc/BUILD.MAC.md)

[Linux](doc/BUILD.LINUX.md)

### Configuration Types
| CMake                      | Description                                                                         |
|:---------------------------|:------------------------------------------------------------------------------------|
| Debug                      | A debug build linked against debug libraries                                        |
| OptDebug                   | A debug build linked with release libraries                                         |
| RelWithDebInfo             | A release optimized build with asserts linked with release libraries                |
| Release                    | A release optimized build linked with release libraries                             |

### Build Options

| CMake                      | Description                                                                         | Default |
|:---------------------------|:------------------------------------------------------------------------------------|---------|
| BUILD_VIEWER               | Build viewer binaries                                                               | ON      |
| BUILD_APPEARANCE_UTIL      | Build appearance utility                                                            | OFF     |
| BUILD_TESTING              | Build test binries.                                                                 | OFF     |
| PACKAGE                    | Build installer packages when viewer build enabled                                  | ON      |
| USE_OPENAL                 | Build with support for the OpenAL audio engine                                      | ON      |

## Contribute

Help make Second Life better! You can get involved with improvements by filing bugs, suggesting enhancements, submitting
pull requests and more. See the [CONTRIBUTING][] and the [open source portal][] for details.

[Second Life]: https://secondlife.com/
[download]: https://secondlife.com/support/downloads/
[tpv]: http://wiki.secondlife.com/wiki/Third_Party_Viewer_Directory
[open source portal]: http://wiki.secondlife.com/wiki/Open_Source_Portal
[contributing]: https://github.com/secondlife/viewer/blob/main/CONTRIBUTING.md

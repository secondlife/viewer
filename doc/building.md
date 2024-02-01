# Building the Viewer
Building the viewer is generally a straight forward process provided you've satisfied a few pre-requisites.  This build guide aims to assist you in building the viewer from a clean environment. 

## Pre-requisite software
You're going to need the following software to compile the viewer.  This list applies to all platforms.
* [CMake](https://cmake.org/download/)
* [Git](https://git-scm.com/downloads)
* [Python 3.7 or higher](https://www.python.org/downloads/) - Be sure to "Add Python to PATH" on Windows
* [Autobuild](https://wiki.secondlife.com/wiki/Autobuild)

Depending on your platform, you may already satisfy some of these requirements.  Please see below for platform specific pre-requisites.

### Windows pre-requisite software
On Windows, you will need to install [Visual Studio 2022](https://visualstudio.microsoft.com/vs/older-downloads/) with the "Desktop development with C++" workload.

Read [Building on Windows](building-windows.md) for critical information on Windows-specific instructions.

### macOS pre-requisite software
On macOS, you will need to install [Xcode](https://apps.apple.com/us/app/xcode/id497799835?mt=12).

Read [Building on macOS](building-macos.md) for critical information on macOS-specific instructions.

## Platform independent instructions
Getting started with building the viewer is generally straightforward on supported platforms.  Beyond a few documented issues specific to each platform, the process to configure the viewer for building is generally the same.

### Step 1 - cloning the viewer
The viewer's source code is hosted on [GitHub](https://github.com/secondlife/viewer), and may be cloned with the following command:

`git clone https://github.com/secondlife/viewer.git`

### Step 2 - setting build variables
Scripts are provided to assist in this process for your convenience.  You may find these scripts in the [scripts](../scripts/building) directory in this repository.  Operating system specific instructions for these scripts are included in the above build guides.  For more advanced setups, continue reading on.

For advanced setups, you will need to set some build variables prior to building.

#### Step 2.1 - cloning the build variables repository
In a separate directory outside this repository (preferably right next to the `viewer` repository), execute the following:

```
git clone https://github.com/secondlife/build-variables
cd build_variables
git checkout viewer
```
If you examine the contents of the `build-variables/variables` file you'll see the details of what configuration values are being set. These variables are used by the CMake configuration step which is invoked by `autobuild`.

#### Step 2.2 - sourcing the build variables
From within the viewer repository you can source the variables config file during the `source_environment` step like so:

`autobuild source_environment ../build_variables/variables`

Instead of sourcing the variables file explicitly, and easier for long term development, you can create an `AUTOBUILD_VARIABLES_FILE` environment variable by putting this in your `.bashrc` file (when using a bash terminal), or more globally by adding it to the Windows environment variables configuration:

`export AUTOBUILD_VARIABLES_FILE=<path-to-your-variables-file>`

PowerShell users on Windows can do this with the following command:

`$env:AUTOBUILD_VARIABLES_FILE='<path-to-your-variables-file>'`

With that done you can just:

`autobuild source_environment`

### Step 3 - configuring

Once your environment is set, you can now configure the project with `autobuild`.  There are multiple paths to do this.  For Linden builds with proprietary dependencies, make sure you have configured a GitHub Personal Access Token with the `AUTOBUILD_GITHUB_TOKEN` environment variable that can access internal 3p packages and run:

`autobuild configure`

For open source builds, you need to provide a valid open source configuration.  You may use either `ReleaseOS` or `RelWithDebInfoOS`.  In this example, we'll use `RelWithDebInfoOS` as it also generates debug symbols with the build:

`autobuild configure -c RelWithDebInfoOS -A 64`

The `-c` argument determines which build configuration to create, generally either `RelWithDebInfoOS` or `ReleaseOS`. You can omit the option if you set the `AUTOBUILD_CONFIGURATION` environment variable to the one you want. `-A` may be either `64` or `32`, depending on which you intend to build. You can omit that option if you set the `AUTOBUILD_ADDRSIZE` environment variable accordingly.  Note that macOS only supports 64-bit builds.

Please be patient: the `autobuild configure` command silently fetches and installs required `autobuild` packages, and some of them are large.

### Step 4 - building

When configuring completes, you can either build with the specific IDE for your platform, or from the command line.

#### Autobuild options
For help on `configure` options, type:

`autobuild configure --help`

The `BUILD_ID` is only important for a viewer you intend to distribute. For a local test build, it doesn't matter: it only needs to be distinct. If you omit `--id` (as is typical), `autobuild` will invent a `BUILD_ID` for you.

For the Linden viewer build, this usage:

`autobuild configure [autobuild options]... -- [other options]...`

passes `[other options]` to CMake. This can be used to override different CMake variables, e.g.:

`autobuild configure [autobuild options]... -- -DSOME_VARIABLE:BOOL=TRUE`

The set of applicable CMake variables is still evolving. Please consult the CMake source files in `indra/cmake`, as well as the individual `CMakeLists.txt` files in the `indra` directory tree, to learn their effects. 
# Building the Viewer on macOS
Building the viewer on macOS has some special considerations compared to Windows and our [general build instructions](building.md).

If you haven't, read our [general build instructions](building.md) before proceeding.

## Installing pre-requisites
On macOS, most pre-requisites are taken care with a few exceptions.  You will need to individually install the following:

* [CMake](https://cmake.org/download/)
* [Xcode](https://apps.apple.com/us/app/xcode/id497799835?mt=12)
* [Autobuild](https://wiki.secondlife.com/wiki/Autobuild)

There's multiple ways to install our pre-requisites on macOS.  This guide assumes you're doing this on a clean system without a package manager such as Homebrew.

### Installing Autobuild

Xcode generally uses the system installed Python when executing our python scripts - so it's recommended to use the system provided `pip3` command for installation of python tools.  This is required to successfully build the viewer.  To do this, you can execute:

```Shell
pip3 install autobuild
pip3 install llbase
```

#### An important note about pip3 and paths
By default, executables installed with the `pip3` command **will not** be added to the path on a clean system.  This is generally easy to fix on macOS though, assuming the Xcode installed version of `pip3`.  Simply do this in your terminal:

```Shell
export PATH=$(python3 -m site --user-base)/bin:$PATH
```

To make this persist across terminal sessions:
```Shell
echo "export PATH=$(python3 -m site --user-base)/bin:$PATH" >> ~/.zshrc
source ~/.zshrc
```
Note that by default, .zshrc does not exist on macOS.

From there `autobuild` will be available to execute in your terminal.

### Installing CMake

If you installed CMake from Kitware's website, by default CMake will not be available from the command line.  To remedy this, run the following command:

```Shell
sudo "Applications/CMake.app/Contents/bin/cmake-gui" --install
```
This will make the `cmake` command available from the terminal.  This is required in order to generate build files.

## Setting up build environment variables
A script has been provided in order to simplify setting up your terminal session to build the viewer.  You can find it in [scripts/building/setup_viewer_build.sh](../scripts/building/setup_viewer_build.sh).

In order to use this script, you must `source` it like so:
```Shell
source viewer/scripts/building/setup_viewer_build.sh
```
This will clone (if necessary) the `build-variables` repository which can be found on [GitHub](https://github.com/secondlife/build-variables).  It will also set checkout the `viewer` branch, and setup environment variables local to your current terminal session.  If you have access to proprietary packages, you can set the GitHub access token and build configuration to reflect this like so:

```Shell
export AUTOBUILD_CONFIGURATION=RelWithDebInfo
export AUTOBUILD_GITHUB_TOKEN="<your personal access token here>"
```
Do not modify the build setup script with these values.  Simply executing these commands in your terminal session will override the specific environment variables that were provided by the setup script for that session.

## Generating build files
After you've installed all pre-requisites, you can now generate build files.

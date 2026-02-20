# Building on macOS

## Step 1: Install Dependencies and Tools

### Install Xcode and Homebrew

* [Xcode](https://developer.apple.com/xcode/)
* [Homebrew](https://brew.sh/)

### Install Homebrew dependencies

Now we're going to install required build tools from Homebrew.

```
brew install git cmake zip unzip curl pkgconf automake autoconf autoconf-archive gettext libtool
```

## Step 2: Checkout Viewer Code
Open a Terminal and checkout the viewer source code:

```git clone https://github.com/secondlife/viewer.git```

## Step 3: Setup Virtual Environment and Python dependencies
```
cd viewer
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

## Step 4: Configure and install vcpkg dependencies
Switch to the viewer repository you just checked out and run cmake to configure:

```
cmake -S indra --preset xcode-os
```

The --preset argument determines which build configuration to create, generally either an individual build configuration or a multi-config IDE such as Visual Studio or Xcode.

To list availiable presets:

```cmake -S indra --list-presets```


For the Linden viewer build, this usage:

```cmake -S indra --preset xcode-os [other options]...```

passes [other options] to CMake. This can be used to override different CMake variables, e.g.:

```cmake -S indra --preset xcode-os -DSOME_VARIABLE:BOOL=TRUE```

The set of applicable CMake variables is still evolving. Please consult the CMake source files in indra/cmake, as well as the individual CMakeLists.txt files in the indra directory tree, to learn their effects.

## Step 5: Build
When that completes, you can either build within Xcode or from the Terminal:

### Xcode:
The command below will open the generated xcodeproj in Xcode

```
open ./build-Darwin-xcode-os/SecondLife.xcodeproj
```

### Terminal:
Build by running the following command:

```
cmake --build build-Darwin-xcode-os --config Release
```

the resulting viewer executable will be at:

```
build-Darwin-xcode-os/newview/<CONFIGURATION>/SecondLife.app
```



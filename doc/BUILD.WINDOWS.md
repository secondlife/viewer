# Building on Windows

## Step 1: Install Build Tools

* [CMake](https://cmake.org/download/)
* [Git for Windows](https://git-scm.com/install/windows)
* [Visual Studio 2022 or 2026](https://visualstudio.microsoft.com/vs/community/) - Select "Desktop development with C++" workload
* [Python 3.10+](https://www.python.org/downloads/) - Be sure to "Add Python to PATH"

### Intermediate Check

Confirm things are installed properly so far by typing the following in a Terminal:

```
cmake --version
python --version
git --version
```

If everything reported sensible values and not "Command not found" errors, then you are in good shape!

## Step 2: Checkout Viewer Code
Open a `Powershell` from the `Start Menu` and checkout the viewer source code:

```git clone https://github.com/secondlife/viewer.git```

## Step 3: Setup Virtual Environment and Python dependencies
```
cd viewer
python3 -m venv .venv
.\.venv\Scripts\Activate.ps1
pip install -r requirements.txt
```

## Step 4: Configure and install vcpkg dependencies
Switch to the viewer repository you just checked out and run cmake to configure and install dependencies:

```
cmake -S indra --preset vs2022-os
```

The --preset argument determines which build configuration to create, generally either an individual build configuration or a multi-config IDE such as Visual Studio.

To list availiable presets:

```cmake -S indra --list-presets```


For the Linden viewer build, this usage:

```cmake -S indra --preset vs2022-os [other options]...```

passes [other options] to CMake. This can be used to override different CMake variables, e.g.:

```cmake -S indra --preset vs2022-os -DSOME_VARIABLE:BOOL=TRUE```

The set of applicable CMake variables is still evolving. Please consult the CMake source files in indra/cmake, as well as the individual CMakeLists.txt files in the indra directory tree, to learn their effects.

## Step 5: Build
When that completes, you can either build within Visual Studio or from the command line:

### Visual Studio:
The command below will open the generated solution in Visual Studio

```
explorer.exe .\build-Windows-vs2022-os\SecondLife.sln
```

### Command Line:
Build by running:

```
cmake --build build-Windows-vs2022-os --config Release
```

the resulting viewer executable will be at:

```
build-Windows-vs2022-os/newview/<CONFIGURATION>/SecondLifeViewer.exe
```



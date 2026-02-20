# Building on Linux

## Install Dependencies

<details>
<summary>Arch</summary>

```
sudo pacman -Syu automake autoconf base-devel cmake fontconfig git glib2-devel gstreamer gst-plugins-base-libs ninja libglvnd libvlc libx11 pkgconf python wayland
```

</details>

<details>
<summary>Debian</summary>

#### Debian 12+

```
sudo apt install \
autoconf autoconf-archive automake bison build-essential cmake curl flex gettext \
libasound2-dev libaudio-dev libdbus-1-dev libdbus-1-dev libdecor-0-dev libdrm-dev \
libegl1-mesa-dev libfribidi-dev libgbm-dev libgl1-mesa-dev libgles2-mesa-dev \
libgstreamer-plugins-base1.0-dev libgstreamer1.0-dev libibus-1.0-dev libjack-dev \
libosmesa6-dev libpipewire-0.3-dev libpulse-dev libsndio-dev libtext-unidecode-perl \
libthai-dev libtool libudev-dev libunwind-dev liburing-dev libvlc-dev libwayland-dev \
libx11-dev libxcursor-dev libxext-dev libxfixes-dev libxft-dev libxi-dev libxinerama-dev \
libxkbcommon-dev libxrandr-dev libxss-dev libxtst-dev linux-libc-dev ninja-build \
pkgconf tar tex-common texinfo unzip zip 
```

</details>

<details open>
<summary>Ubuntu</summary>

#### Ubuntu 22.04+
```
sudo apt install \
autoconf autoconf-archive automake bison build-essential cmake curl flex gettext \
libasound2-dev libaudio-dev libdbus-1-dev libdbus-1-dev libdecor-0-dev libdrm-dev \
libegl1-mesa-dev libfribidi-dev libgbm-dev libgl1-mesa-dev libgles2-mesa-dev \
libgstreamer-plugins-base1.0-dev libgstreamer1.0-dev libibus-1.0-dev libjack-dev \
libosmesa6-dev libpipewire-0.3-dev libpulse-dev libsndio-dev libtext-unidecode-perl \
libthai-dev libtool libudev-dev libunwind-dev liburing-dev libvlc-dev libwayland-dev \
libx11-dev libxcursor-dev libxext-dev libxfixes-dev libxft-dev libxi-dev libxinerama-dev \
libxkbcommon-dev libxrandr-dev libxss-dev libxtst-dev linux-libc-dev ninja-build \
pkgconf tar tex-common texinfo unzip zip 
```

</details>

<details>
<summary>Fedora/RHEL</summary>

#### AlmaLinux 10
```
sudo dnf group install "Development Tools"
sudo dnf install cmake fontconfig-devel git glib2-devel gstreamer1-devel gstreamer1-plugins-base-devel libX11-devel mesa-libOSMesa-devel libglvnd-devel ninja-build python3 vlc-devel wayland-devel
```
> [!NOTE]
> You may need to enable the EPEL repository for some packages `sudo dnf install epel-release`

#### Fedora 44+
```
sudo dnf install @development-tools @c-development cmake fontconfig-devel git glib-devel gstreamer1-devel gstreamer1-plugins-base-devel libX11-devel mesa-compat-libOSMesa-devel libglvnd-devel ninja-build python3 vlc-devel wayland-devel
```

To build with clang instead of gcc, also install:
```
sudo dnf install clang lld
```

</details>

<details>
<summary>OpenSUSE</summary>

#### Tumbleweed
```
sudo zypper in -t pattern devel_basis devel_C_C++
sudo zypper install cmake fontconfig-devel git glib2-devel gstreamer-devel gstreamer-plugins-base-devel libglvnd-devel libX11-devel ninja Mesa-libGL-devel python3 vlc-devel wayland-devel
```

</details>

## Create development folders
```
mkdir -p ~/code/secondlife
cd ~/code/secondlife
```

## Checkout viewer code
```
git clone https://github.com/secondlife/viewer.git
cd ~/code/secondlife/viewer
```

## Setup Virtual Environment and Python dependencies
```
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

## Configure and install vcpkg dependencies
Switch to the viewer repository you just checked out and run cmake to configure:


### GCC
```
cmake -S indra --preset ninja-os
```

### Clang (faster build; less stable)
```
cmake -S indra --preset ninja-os -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_LINKER_TYPE=LLD
```

> [!TIP]
> If you encounter errors due to cmake being out of date you can fetch it from pip!
>
> `pip install --upgrade cmake ninja`

The --preset argument determines which build configuration to create, generally either an individual build configuration or a multi-config generator such as Ninja.

To list availiable presets:

```cmake -S indra --list-presets```


For the Linden viewer build, this usage:

```cmake -S indra --preset ninja-os [other options]...```

passes [other options] to CMake. This can be used to override different CMake variables, e.g.:

```cmake -S indra --preset ninja-os -DSOME_VARIABLE:BOOL=TRUE```

The set of applicable CMake variables is still evolving. Please consult the CMake source files in indra/cmake, as well as the individual CMakeLists.txt files in the indra directory tree, to learn their effects.

## Build Viewer
Now switch to the `indra` directory and run the following command:

```
cmake --build --preset ninja-os-release
```

> [!NOTE]
> If the above was successful you should find the viewer package in `viewer/build-Linux-ninja-os/newview/Release`

## Troubleshooting
- If you encounter warnings, try adding `-DGCC_DISABLE_FATAL_WARNINGS=TRUE` or `-DCLANG_DISABLE_FATAL_WARNINGS=TRUE` to the configure command

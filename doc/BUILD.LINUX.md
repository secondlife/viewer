# Building on Linux

## Install Dependencies

### AlmaLinux 10
```
sudo dnf group install "Development Tools"
sudo dnf install cmake fontconfig-devel git glib2-devel gstreamer1-devel gstreamer1-plugins-base-devel libX11-devel mesa-libOSMesa-devel libglvnd-devel ninja-build python3 vlc-devel wayland-devel
```
> [!NOTE]
> You may need to enable the EPEL repository for some packages `sudo dnf install epel-release`

### Arch
```
sudo pacman -Syu base-devel cmake fontconfig git glib2-devel gstreamer gst-plugins-base-libs ninja libglvnd libvlc libx11 python wayland
```

### Debian - Ubuntu 22.04 - Ubuntu 24.04
```
sudo apt install build-essential cmake git libfontconfig-dev libglib2.0-dev libglvnd-dev libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libosmesa6-dev libvlc-dev libwayland-dev libx11-dev ninja-build python3 python3-venv
```

### Fedora
```
sudo dnf install @development-tools @c-development cmake fontconfig-devel git glib-devel gstreamer1-devel gstreamer1-plugins-base-devel libX11-devel mesa-compat-libOSMesa-devel libglvnd-devel ninja-build python3 vlc-devel wayland-devel
```

### OpenSUSE Tumbleweed
```
sudo zypper in -t pattern devel_basis devel_C_C++
sudo zypper install cmake fontconfig-devel git glib2-devel gstreamer-devel gstreamer-plugins-base-devel libglvnd-devel libX11-devel ninja Mesa-libGL-devel python3 vlc-devel wayland-devel
```

## Create development folders
```
mkdir -p ~/code/secondlife
cd ~/code/secondlife
```

## Setup Virtual Environment and install Autobuild
```
python3 -m venv .venv
source .venv/bin/activate
pip install autobuild llsd
```
> [!NOTE]
> If the above was successful you should receive output similar to `autobuild 3.10.2` when running `autobuild --version`

## Setup Build Variables
```
git clone https://github.com/secondlife/build-variables.git
export AUTOBUILD_VARIABLES_FILE=~/code/secondlife/build-variables/variables
```

## Clone Viewer
```
git clone https://github.com/secondlife/viewer.git
cd ~/code/secondlife/viewer
```

## Configure Viewer

### GCC
```
autobuild configure -c ReleaseOS -A64
```

### Clang
```
autobuild configure -c ReleaseOS -- -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_LINKER_TYPE=LLD
```

## Build Viewer
```
autobuild build -c ReleaseOS --no-configure
```

> [!NOTE]
> If the above was successful you should find the viewer package in `viewer/build-linux-x86_64/newview`

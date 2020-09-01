# Griddly

A cross platform grid-based research environment that is designed to be able to reproduce grid-world style games.
[![Documentation Status](https://readthedocs.org/projects/griddly/badge/?version=latest)](https://griddly.readthedocs.io/en/latest/?badge=latest)

Build Status
| OS       |  Status  |
|----------|----------|
| Linux    | [![Build Status](https://dev.azure.com/chrisbam4d/Griddly/_apis/build/status/Bam4d.Griddly?branchName=develop&jobName=Linux)](https://dev.azure.com/chrisbam4d/Griddly/_build/latest?definitionId=1&branchName=develop)  |
| Windows  | [![Build Status](https://dev.azure.com/chrisbam4d/Griddly/_apis/build/status/Bam4d.Griddly?branchName=develop&jobName=Windows)](https://dev.azure.com/chrisbam4d/Griddly/_build/latest?definitionId=1&branchName=develop)  |
| MacOS    | [![Build Status](https://dev.azure.com/chrisbam4d/Griddly/_apis/build/status/Bam4d.Griddly?branchName=develop&jobName=MacOS)](https://dev.azure.com/chrisbam4d/Griddly/_build/latest?definitionId=1&branchName=develop)  |

# Community
Join the Discord community for help and to talk about what you are doing with Griddly!

[![Discord Chat](https://img.shields.io/discord/728222413397164142.svg)](https://discord.gg/xuR8Dsv)

# Documentation

This documentation is only for building from source.

Full documentation can be found here:
[griddly.readthedocs.org](https://griddly.readthedocs.org)

## Installing Without Building Locally

### Python

To install Griddly without building from source just install from pypi:

```
pip install griddly
```

### Java (Coming Soon) 

Java library support will be developed using [JavaCpp](https://github.com/bytedeco/javacpp) 

### Julia 
A simplest way is coming, hopefully you will just have to do in the julia REPL
```julia
pkg> add Griddly
```
For now please look at the Julia locally section on how to [Build Julia locally](#build-julia-locally).

# Building locally

Firstly sync up the git submodules:

```
git submodule init
git submodule update
```

```
cmake . -DCMAKE_BUILD_TYPE={Debug|Release}
cmake --build .
```

Artifacts can then be found in {Debug|Release}/bin

## Tests

Test can be run with:
```
ctest .
```

## Python Installer

To install the built libraries with python you need to do one of the following (depending if your built the binaries with `Debug` or `Release`)

The installer should copy the following files into the `python/griddly` directory:

* `resources/*` -> `python/griddly/resources`
* `{Release|Debug}/python_griddly.cpython*` -> `python/griddly/libs`

### Debug

```
cd python
python setup.py develop
```

OR

```
cd python
pip install -e .
```

### Release

```
cd python
python setup.py install
```

## Prerequisites

### Ubuntu
```
wget -qO - http://packages.lunarg.com/lunarg-signing-key-pub.asc | sudo apt-key add -
sudo wget -qO /etc/apt/sources.list.d/lunarg-vulkan-bionic.list http://packages.lunarg.com/vulkan/lunarg-vulkan-bionic.list
sudo apt update
sudo apt install vulkan-sdk
```

### Windows

1. Install [cmake](https://cmake.org/download/)
2. Install MinGW (posix 8.1.0) *or* MSVC
3. Install [Vulkan](https://vulkan.lunarg.com/sdk/home) 

### MacOS

1. Install xcode CLI tools
```
xcode-select --install
```
2. Install Brew 
```
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install.sh)"
```
3. Install cmake
```
brew install cmake
```
4. Install [Vulkan](https://vulkan.lunarg.com/sdk/home) 

## Build Julia Locally 

You first need to complete all the Building Locally step.

### Prerequisites

1. Install [Julia](https://julialang.org/downloads/oldreleases/) v1.3.1 at least

2. In a Julia REPL in pkg mode (`]` at the REPL) you add  `CxxWrap` v0.11.0 at least
```julia
pkg> add CxxWrap
```
For more information about CxxWrap you can check [here](https://github.com/JuliaInterop/CxxWrap.jl)
Note that CxxWrap v0.10 and later depends on the `libcxxwrap_julia_jll` [JLL package](https://julialang.org/blog/2019/11/artifacts/) to manage the `libcxxwrap-julia` binaries.

3. Once you get `CxxWrap` installed you need to build the `libcxxwrap-julia` binaries. See the [libcxxwrap-julia Readme](https://github.com/JuliaInterop/libcxxwrap-julia) for information on how to build this library yourself and force CxxWrap to use your own version.

4. Once you have built `libcxxwrap-julia` in pkg mode (`]` in the REPL) you need to do
```julia
pkg> build CxxWrap
```
Then close and restart your REPL.

5. In pkg mode (`]` at the REPL) you add `Makie` v0.10.0 at least
```julia
pkg> add Makie
```

Note: If you get troubles to install those look at the [Julia locally troubles](#troubles-building-julia-locally) section.

### Linux building

```
cmake . -DCMAKE_BUILD_TYPE={Debug|Release} -DBUILD_JULIA=ON -DJulia_EXECUTABLE="path/to/your/julia.exe" -DJlCxx_DIR="path/to/your/libcxxwrap/build"
cmake --build .
```
Artifacts can then be found in {Debug|Release}/bin

Note: The build of `libcxxwrap-julia` binaries should be the same than the build type you want to do here

### Windows building (with Microsoft Visual Studio 2019)

1. Open the project with Microsoft Visual Studio 2019
2. From the built-in CMake support, see the [Visual docs](https://docs.microsoft.com/en-us/cpp/build/customize-cmake-settings?view=vs-2019) for more infos, you can configure the `Julia_Prefix` (which you set to "path/to/your/julia.exe") and `JlCxx_DIR`(which you set to "path/to/libcxxwrap-julia/out/build/build_type") option.
3. Still with the built-in CMake support you can choose your build option (mainly Release and the msvc_x64_x64 Toolset or Debug and the msvc_x64_x64 Toolset) and in the CMake toolchain file you can add : `-DBUILD_JULIA=ON`
4. Save your settings, and right-click on the CMakeList.txt file and choose build

### MacOS

Work In Progress

### Activate the Griddly Package

1. In the shell mode (hit `;` at the REPL)
```julia
shell> cd path/to/the/folder/Griddly/julia
```
2. In pkg mode (hit `]` at the REPL)
```julia
pkg> activate Griddly
```
You should see ```(Griddly)pkg>``` in the REPL. From now on you can do ```using Griddly```

3. In the shell mode (hit `;` at the REPL)
```julia
shell> cd ..
```
You are now back to the main directory

4. Launch a test file, in the REPL:
```julia
include("julia/Griddly/test/play_gvgai_raw.jl")
```

5. If you want to quit the pkg mode with Griddly, just do in the pkg mode
```julia
(Griddly)pkg> activate
```
Which should now look like:
```julia
(v.1.3)pkg>
```

### Troubles building Julia locally

1. Trouble to build correctly `libcxxwrap-julia` with Visual Studio 2019

-> Open Microsoft Visual Studio 2019

-> Choose the Clone a repository option

-> Add https://github.com/JuliaInterop/libcxxwrap-julia.git to the Repository field

-> From the built-in CMake support, see the [Visual docs](https://docs.microsoft.com/en-us/cpp/build/customize-cmake-settings?view=vs-2019) for more infos, you can configure the `Julia_Prefix` (which you set to "path/to/your/julia.exe")

-> Still from the built-in CMake support, can choose your build option (mainly Release and the msvc_x64_x64 Toolset)

-> Save your settings and in Solution Explorer right-click on the CMakeList.txt file and choose build

-> Create a file named Overrides.toml and write in it
```
[3eaa8342-bff7-56a5-9981-c04077f7cee7]
libcxxwrap_julia = "C:/path/to/your/libcxxwrap-julia/out/build/build_type"
```
-> Save this file in the following location: `C:/Users/Username/.julia/artifacts`

-> Open a julia REPL

-> Enter Pkg mode (`]` in the REPL) and do
```pkg
build CxxWrap
```
-> Close Julia

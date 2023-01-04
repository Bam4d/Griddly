# <p align="center">![Griddly](resources/logo.png)</p>
<div align="center">

[![Discord Chat](https://img.shields.io/discord/728222413397164142.svg)](https://discord.gg/xuR8Dsv)
[![PyPI version](https://badge.fury.io/py/griddly.svg)](https://badge.fury.io/py/griddly)

</div>



A heavily optimized and flexible grid-world game engine for Game AI research.

# ![WHOOSH!!](resources/images/gvgai/oryx/tree2.png) Some Environments

<div>
  <a href="https://griddly.readthedocs.io/en/latest/games/Spider_Nest/index.html"><img style="width: 30%" src="docs/games/Spider_Nest/img/Spider_Nest-level-Isometric-0.png"/></a>
  <a href="https://griddly.readthedocs.io/en/latest/games/GriddlyRTS/index.html"><img style="width: 30%" src="docs/games/GriddlyRTS/img/GriddlyRTS-level-Block2D-3.png"/></a>
  <a href="https://griddly.readthedocs.io/en/latest/games/Clusters/index.html"><img style="width: 30%" src="docs/games/Clusters/img/Clusters-level-Sprite2D-3.png"/></a>
</div>

[Check out the full list of pre-built environments here](https://griddly.readthedocs.io/en/latest/games/index.html)

# ![BOOM](resources/images/gvgai/oryx/sparkle3.png) Some features
* Design and build game mechanics using a simple YAML-based language: [GDY](https://griddly.readthedocs.io/en/latest/getting-started/gdy/index.html)
* **Fast execution speed** environments can run up to 70k FPS on a single thread.
* Don't worry about complex efficient rendering code, just define the images for each game object and Griddly passes this to the GPU.
* Supports **Single Agent**, **Multi Agent** and **RTS** environment interfaces.
* **[Multiple observervation modes](https://griddly.readthedocs.io/en/latest/getting-started/observation%20spaces/index.html)**:
  * Vectorized
    * Configurable one-hot encodings for each x,y coordinate
  * Rendered 
    * Isometric, sprites and simple shapes
    * We also let you use **custom shaders** to render however you want!
  * State Maps
    * Description of the entire environment including objects, locations and variables.
  * Event History
    * Descriptions of actions that have happened in each step.
  * Configurable partial observability
* **Copyable Forward Models**
  * Game state can be cloned at any point in time. This is super useful for many Algorithms. For example Mont-Carlo Tree Search.
* **[Procedural Content Generation Interfaces](https://griddly.readthedocs.io/en/latest/getting-started/procedural%20content%20generation/index.html)**


* Reinforcement Learning with [RLLib](https://griddly.readthedocs.io/en/latest/rllib/intro/index.html)
  * Integration with [Wandb](https://griddly.readthedocs.io/en/latest/rllib/intro/index.html#weights-and-biases-wandb)


# ![Community](resources/images/oryx/oryx_fantasy/avatars/doggo1.png) Community
Join the [Discord community](https://discord.gg/xuR8Dsv) for help and to talk about what you are doing with Griddly!



# ![Documentation](resources/images/oryx/oryx_fantasy/bookshelf-1.png) Documentation

Full documentation can be found here:
[griddly.readthedocs.org](https://griddly.readthedocs.org)

# ![Tutorials](resources/images/gvgai/oryx/book1.png) ![Tutorials](resources/images/gvgai/oryx/book2.png) ![Tutorials](resources/images/gvgai/oryx/book3.png) Tutorials

The most awesome part of Griddly is the ability to easily customize and build your own research environments. To make this extra easy we've written lots of tutorials:

## [Making A Simple Game (Sokoban)](https://griddly.readthedocs.io/en/latest/tutorials/GDY/index.html)
  * This tutorial takes you through how to create a simple game environment using GDY.
  
## Game Mechanics
  * [Proximity Triggers](https://griddly.readthedocs.io/en/latest/tutorials/Proximity%20Triggers/index.html)
    * Configure interactions between objects based on their distance.
  * [Projectiles](https://griddly.readthedocs.io/en/latest/tutorials/Projectiles/index.html)
    * Objects that move under their own power.
  * [Level Design](https://griddly.readthedocs.io/en/latest/tutorials/Level%20Design/index.html)
    * How to design levels for single and multi-agent environments.
  * [Stochasticity](https://griddly.readthedocs.io/en/latest/tutorials/Stochasticity/index.html)
    * How to make environments with stochastic mechanics.
  * [A* Search](https://griddly.readthedocs.io/en/latest/tutorials/AStarSearch/index.html)
    * Use A* pathfinding for mobs in Griddly


## [Custom Shaders](https://griddly.readthedocs.io/en/latest/tutorials/Custom%20Shaders/index.html)
  * Customize rendering for environments using SPIR-V shaders.
<div>
  <div style="text-align: center">
    <h3>Global Lighting</h3>
    <img src="python/examples/Custom Shaders/Global%20Lighting/global_lighting.gif"/>
  </div>
  <div style="text-align: center">
    <h3>Health Bars</h3>
    <img src="python/examples/Custom Shaders/Health%20Bars/health_bars.gif"/>
  </div>
  <div style="text-align: center">
    <h3>Per-Object Lighting</h3>
    <img src="python/examples/Custom Shaders/Object%20Lighting/object_lighting.gif"/>
  </h3>
</div>

# ![Installing](resources/images/gvgai/oryx/hammer1.png) Installing

<div align="center">

```
pip install griddly
``` 

Build Status
| OS       |  Status  |
|----------|----------|
| Linux    | [![Build Status](https://dev.azure.com/chrisbam4d/Griddly/_apis/build/status/Bam4d.Griddly?branchName=develop&jobName=Linux)](https://dev.azure.com/chrisbam4d/Griddly/_build/latest?definitionId=1&branchName=develop)  |
| Windows  | [![Build Status](https://dev.azure.com/chrisbam4d/Griddly/_apis/build/status/Bam4d.Griddly?branchName=develop&jobName=Windows)](https://dev.azure.com/chrisbam4d/Griddly/_build/latest?definitionId=1&branchName=develop)  |
| MacOS    | [![Build Status](https://dev.azure.com/chrisbam4d/Griddly/_apis/build/status/Bam4d.Griddly?branchName=develop&jobName=MacOS)](https://dev.azure.com/chrisbam4d/Griddly/_build/latest?definitionId=1&branchName=develop)  |

</div>


# ![Building Locally](resources/images/gvgai/oryx/staff1.png) Building locally

Firstly you will need to configure the repo for the build.

You can do this using the `configure.sh` script in the root directory.


## Web Assembly Binaries

Build a debug version of the web-assembly binaries.


### Manually

You can run all the build commands manually 

```bash

# Download the build requirements
conan install deps/wasm/conanfile_wasm.txt --profile:host deps/wasm/emscripten.profile --profile:build default -s build_type=Debug --build missing -if build_wasm

# Configure the cmake build
cmake . -B build_wasm -GNinja -DWASM=ON -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake

# Run the build
cmake --build build_wasm --config Debug
```

### Using [Cmake Presets]()

We provide two presets for CMake to use to develop locally using your IDE (provided it supports CMakePresets)

```bash

# Download the build requirements
./configure.sh -p=WASM -b=Debug

# Configure build with preset
cmake --preset="Debug WASM"

# Build with preset
cmake --build build_wasm --config Debug
```

## Native Binaries

### Manually

Build native griddly binaries:

```bash

# Download the build requirements
conan install deps/conanfile.txt --profile default --profile deps/build.profile -s build_type=Debug --build missing -if build

# Configure the cmake build
cmake . -B build -GNinja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake

# Run the build
cmake --build build --config Debug
```

### Using [Cmake Presets]()

We provide two presets for CMake to use to develop locally using your IDE (provided it supports CMakePresets)

```bash

# Download the build requirements
./configure.sh -b=Debug

# Configure build with preset
cmake --preset="Debug Native"

# Build with preset
cmake --build build --config Debug
```

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

We use [Conan](https://conan.io) to install all build dependencies and libraries used to compile Griddly.

You will need to set up a python environment of your choice and then install conan using:

```
pip install conan
```

### Debug Layers for Vulkan

When compiling using Debug, you will need the install vulkan validation layers which are part of the [vulkan sdk](https://vulkan.lunarg.com/).
Otherwise you will run into errors like:
```
[2023-01-04 15:21:54.412] [warning] Required vulkan layer unavailable: VK_LAYER_KHRONOS_validation
[2023-01-04 15:21:54.412] [error] Missing vulkan extensions in driver. Please upgrade your vulkan drivers.
```

### Ubuntu

```
1. Install [cmake](https://cmake.org/download/)
```

### Windows

1. Install [cmake](https://cmake.org/download/)
2. Install [Visual Studio Build Tools](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022)

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

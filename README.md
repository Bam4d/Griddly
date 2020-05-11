# Griddy

A cross platform grid-based research environment that is designed to be able to reproduce any grid-world style game.

Build Status
| OS       |  Status  |
|----------|----------|
| Linux    | [![Build Status](https://dev.azure.com/chrisbam4d/Griddy/_apis/build/status/Bam4d.Griddy?branchName=develop&jobName=Linux)](https://dev.azure.com/chrisbam4d/Griddy/_build/latest?definitionId=1&branchName=develop)  |
| Windows  | [![Build Status](https://dev.azure.com/chrisbam4d/Griddy/_apis/build/status/Bam4d.Griddy?branchName=develop&jobName=Windows)](https://dev.azure.com/chrisbam4d/Griddy/_build/latest?definitionId=1&branchName=develop)  |
| MacOS    | [![Build Status](https://dev.azure.com/chrisbam4d/Griddy/_apis/build/status/Bam4d.Griddy?branchName=develop&jobName=MacOS)](https://dev.azure.com/chrisbam4d/Griddy/_build/latest?definitionId=1&branchName=develop)  |


# Installing Binaries

## Python
Binaries are uploaded to pypi and can be installed with:

```
pip install griddy
```

## Java (Coming Soon) 

Java library support will be developed using [JavaCpp](https://github.com/bytedeco/javacpp) 


# Building locally

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



# GriDdy Yaml (GDY) format

```
actions:
  GATHER:
    behaviours:
    - src:
        type: HARVESTER
        cmd: 
         incr: resources
      dest:
        type: RESOURCE
        cmd: 
         decr: resources

objects:
  Harvester
```

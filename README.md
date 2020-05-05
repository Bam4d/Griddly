# GriddyRTS
A cross platform grid-based research environment that is designed to be compatible with any style of game


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

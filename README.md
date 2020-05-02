[![Build Status](https://dev.azure.com/chrisbam4d/Griddy/_apis/build/status/Bam4d.Griddy?branchName=develop)](https://dev.azure.com/chrisbam4d/Griddy/_build/latest?definitionId=1&branchName=develop)

# GriddyRTS


A cross platform grid-based research environment that is designed to be compatible with any style of game



# Installation notes

# Ubuntu
```
wget -qO - http://packages.lunarg.com/lunarg-signing-key-pub.asc | sudo apt-key add -
sudo wget -qO /etc/apt/sources.list.d/lunarg-vulkan-bionic.list http://packages.lunarg.com/vulkan/lunarg-vulkan-bionic.list
sudo apt update
sudo apt install vulkan-sdk
```

# Windows

1. Install [cmake](https://cmake.org/download/)
2. Install MinGW (posix 8.1.0) 
3. Install [Vulkan](https://vulkan.lunarg.com/sdk/home) 

# MacOS



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

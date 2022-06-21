# Griddly JS

GriddlyJS is built using React and Web Assembly

## Building From Source (including web-assembly)

### Install Emscripten SDK

To compile the web-assembly module for Griddly, you first need to [install emscripten](https://emscripten.org/docs/getting_started/downloads.html).

Emscripten is a compiler toolchain for building web-assembly modules.


### Configure and Build 

Choose Release/Debug for the build type and run the following command:

```shell
./configure.sh && ./build.sh [Release/Debug]
```

Once this is done, you can follow the instructions to Install Dependencies and Run Locally

## Building Webapp Only

GriddlyJS includes a pre-built web-assembly module for the current version of Griddly. 

If you just want to run the web-app locally then run the following commands which sync the resource directories

```
configure.sh
```

## Installing Dependencies

### Node Package Manager

As GriddlyJS is a web project, you must have NPM installed. The easiest way to do this is to [install NVM](https://github.com/nvm-sh/nvm#installing-and-updating) and choose a stable version.

```
nvm install stable && nvm use stable
```


### Install node packages

```shell
npm install
```

## Running Locally

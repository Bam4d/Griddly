#!/bin/bash

cd "$(dirname "$0")"

PLATFORM="x86_64" 
BUILD="Debug"
CONAN_BUILD="missing"

for i in "$@"; do
  case $i in
    -p=*|--platform=*)
      PLATFORM="${i#*=}"
      shift # past argument=value
      ;;
    -b=*|--build-type=*)
      BUILD="${i#*=}"
      shift # past argument=value
      ;;
    -f|--force-rebuild)
      CONAN_BUILD="all"
      shift # past argument=value
      ;;
  esac
done

echo $BUILD
echo $CONAN_BUILD
echo $PLATFORM

if [[ $PLATFORM == "WASM" ]]
then
  conan install deps/wasm/conanfile_wasm.txt --pr:h deps/wasm/emscripten.profile --pr:b default -s build_type=$BUILD --build missing -if build_wasm
else
  conan install deps/conanfile.txt -pr:b=default -pr:h=default -pr:h=deps/build.profile -s build_type=$BUILD --build $CONAN_BUILD -if build
fi


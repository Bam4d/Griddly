[settings]
os=Emscripten
arch=wasm
compiler=clang
compiler.version=14

[tool_requires]
emsdk/3.1.23
ninja/1.11.1

[conf]
tools.cmake.cmaketoolchain:generator=Ninja
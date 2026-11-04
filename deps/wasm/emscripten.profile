[settings]
os=Emscripten
arch=wasm
compiler=clang
compiler.version=17
compiler.libcxx=libc++
compiler.cppstd=17

[tool_requires]
emsdk/3.1.73
ninja/1.11.1

[conf]
tools.cmake.cmaketoolchain:generator=Ninja

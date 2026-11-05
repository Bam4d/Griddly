#!/bin/bash
# Compile + link a benchmark against the Release static lib.
# Usage: ./build.sh bench_tiled   (produces ./bench_tiled binary)
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SRC="$ROOT/benchmarks/$1.cpp"
OBJ="/tmp/$1.o"
BIN="$ROOT/benchmarks/$1"
CONAN="$HOME/.conan2/p"

/usr/bin/c++ -DSPDLOG_COMPILED_LIB -DSPDLOG_FMT_EXTERNAL -DSTB_TEXTEDIT_KEYTYPE=unsigned \
  -I"$ROOT/tests/src" -I"$ROOT/src" \
  -isystem $CONAN/b/yaml-1671e4d863d96/p/include \
  -isystem $CONAN/glmee168c829b0e6/p/include \
  -isystem $CONAN/b/spdlo4e6f82e1301f9/p/include \
  -isystem $CONAN/b/fmt2048a7d2c205e/p/include \
  -isystem $CONAN/b/sdl8c0255475fc2e/p/include \
  -isystem $CONAN/stbe5f1eed0e2d99/p/include \
  -isystem $CONAN/b/gtest5419f336abeeb/p/include \
  -g -O3 -DNDEBUG -std=c++17 \
  -o "$OBJ" -c "$SRC"

/usr/bin/c++ -g -O3 -m64 -o "$BIN" "$OBJ" "$ROOT/Release/bin/libGriddly_static.a" \
  -L$CONAN/b/yaml-1671e4d863d96/p/lib -L$CONAN/b/spdlo4e6f82e1301f9/p/lib \
  -L$CONAN/b/fmt2048a7d2c205e/p/lib -L$CONAN/b/sdl8c0255475fc2e/p/lib \
  -L$CONAN/b/gtest5419f336abeeb/p/lib \
  -Wl,-rpath,$CONAN/b/yaml-1671e4d863d96/p/lib:$CONAN/b/spdlo4e6f82e1301f9/p/lib:$CONAN/b/fmt2048a7d2c205e/p/lib:$CONAN/b/sdl8c0255475fc2e/p/lib \
  "$ROOT/Release/bin/libGriddly_static.a" \
  $CONAN/b/yaml-1671e4d863d96/p/lib/libyaml-cpp.a \
  $CONAN/b/spdlo4e6f82e1301f9/p/lib/libspdlog.a \
  $CONAN/b/fmt2048a7d2c205e/p/lib/libfmt.a \
  $CONAN/b/sdl8c0255475fc2e/p/lib/libSDL3.a \
  $CONAN/b/gtest5419f336abeeb/p/lib/libgmock.a \
  $CONAN/b/gtest5419f336abeeb/p/lib/libgtest.a \
  -lm -lpthread
echo "built $BIN"

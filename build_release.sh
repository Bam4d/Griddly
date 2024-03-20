./configure.sh

conan profile update settings.compiler.libcxx=libstdc++11 default

conan install deps/conanfile.txt --profile default \
 --profile deps/build.profile \
 -s build_type=Release --build missing -if build

cmake . -B build -GNinja -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake

cmake --build build --config Release

pip install -e python

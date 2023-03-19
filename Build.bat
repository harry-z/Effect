@echo off
if not exist Build (
    mkdir Build
)
pushd Build
del /q /s CMakeCache.txt
@echo on
cmake ../ -DCMAKE_BUILD_TYPE:STRING=%1 -G "Visual Studio 16 2019" --no-warn-unused-cli
cmake --build . --config %1
@echo off
popd
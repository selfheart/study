REM rmdir /s /q build-mingw
mkdir build-mingw
cd build-mingw
cmake -G "MinGW Makefiles" ..
cmake --build .

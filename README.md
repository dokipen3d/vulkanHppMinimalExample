I recommend installing the vulkan lunarG sdk which sets all of the environment variables for you and includes the necessary shaderc library prebuilt for you.

then you can install the dependecies with vcpkg

vcpgk install glfw3


then you can invoke cmake with 

```cmake -DCMAKE_TOOLCHAIN_FILE=C:\pathtoyourtoolchainfile\vcpkg.cmake ..```

if you've forgotten where your toolchain file is you can invoke `vcpkg integrate install` and it will print it for you in the terminal.

then to build on windows invoke `cmake --build . --config Release` (other platforms you can just invoke `cmake --build`)
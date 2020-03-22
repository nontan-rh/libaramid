# libaramid ![Build](https://github.com/nontan-rh/libaramid/workflows/Build/badge.svg)

libaramid is a multiprocessing library for C which employs the work-stealing algorithm, fork-join model, and task flow model.
This library focuses on portability and usability in real-world applications.

## Key Features

- Largely compliant with C99 (but a few unavoidable intrinsics)
- Abstracting underlying thread APIs
- Independent of ABI specifics
- Tested on a bunch of environments (including real smartphone devices and Webassembly)
- Error handling API
- Various ways to synchronization
- Custom scheduling
- Capability to use from other languages

## Supported Platforms

Tested platforms and toolchain

| OS      |  CPU  | Compiler   | Note                                |
| ------- | :---: | ---------- | ----------------------------------- |
| Linux   |  x64  | Clang      | Verified using ASAN                 |
| macOS   |  x64  | AppleClang |                                     |
| Windows |  x64  | MSVC       |                                     |
|         |       | MinGW      |                                     |
| iOS     | ARMv8 | AppleClang | Build only (automatic test planned) |
| Android | ARMv8 | GCC        |                                     |
| WASM    |   -   | Emscripten | Tested on Headless Chrome           |

## Getting Started

### CMake

NOTE: Android NDK can utilize CMake and it requires **Ninja**

You can use `ExternalProject` to add to your project.
See [CMake Documentation](https://cmake.org/cmake/help/latest/module/ExternalProject.html)
if you don't know how to use the feature.

```cmake
cmake_minimum_required(VERSION 3.10.2)

ExternalProject_Add(libaramid
    GIT_REPOSITORY https://github.com/nontan-rh/libaramid.git
    GIT_TAG master
    GIT_SHALLOW ON
    SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/libaramid-src"
    BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/libaramid-build"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    TEST_COMMAND ""
)

target_link_libraries(<your_target> aramid)
```

### Carthage

NOTE: You need **cmake** in addition to Xcode and Carthage.

```cartfile
github "nontan-rh/libaramid"
```

### Development

NOTE: In-source build is disabled

#### Generic

```sh
$ mkdir -p build && cd build
$ cmake .. -DCMAKE_BUILD_TYPE=Debug
$ cmake --build .
# for Linux, macOS, Windows
$ ctest
# for Wasm
$ python3 -m http.server --directory test_executable 8080
# -> then visit localhost:8080
```

#### Android

Open `etc/android/TestProject` as a project with AndroidStudio

#### iOS

Open `etc/ios/libaramid/libaramid.xcodeproj` with Xcode

## Third Party Licenses

Build scripts or test drivers in this repository include some third-party software.

| Project                | License      | URL                                              |
| ---------------------- | ------------ | ------------------------------------------------ |
| ios-cmake              | BSD-3-Clause | https://github.com/leetal/ios-cmake              |
| googletest             | BSD-3-Clause | https://github.com/google/googletest             |
| google-toolbox-for-mac | Apache-2.0   | https://github.com/google/google-toolbox-for-mac |

Package managers also fetch some third-party software. Check `build.gradle` and `package.json`.

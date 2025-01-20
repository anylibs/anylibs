include("options.cmake")
include("deps.cmake")

# # Set Zig as the C++ compiler
# set(CMAKE_C_COMPILER "zig")
# set(CMAKE_C_COMPILER_ARG1 "c")

# # Specify target architecture and platform
# set(CMAKE_C_FLAGS "--target=x86_64-windows-gnu")

cmake_minimum_required(VERSION 3.20)
project(anylibs LANGUAGES C)

# compiler options
set(CMAKE_C_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if (MSVC)
    add_compile_options(
        /W4 # warning level 4
        /WX
        /guard:cf # control flow guard
        /sdl # enable security features
        /analyze # enable static analysis
        /permissive- # standards conformance mode
        /utf-8
        $<$<CONFIG:Debug>:/Zi>
        /wd4200 # C4200 zero-sized array in struct/union
        /wd4820 # C4200 padding in struct/union
        # /wd4996 # C4996 unsafe functions
    )
    add_link_options(
        /guard:cf # control flow guard
    )
else()
    if(WIN32)
        set(sanitizers address,undefined)
    else()
        set(sanitizers address,undefined,leak)
    endif()

    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Werror
        $<$<CONFIG:RELEASE>:-D_FORTIFY_SOURCE=2>
        # $<$<CONFIG:DEBUG>:-fsanitize=${sanitizers}>
        $<$<CONFIG:DEBUG>:-fstack-protector-strong>
        $<$<CONFIG:DEBUG>:--param=ssp-buffer-size=4>
        $<$<CONFIG:DEBUG>:-fno-omit-frame-pointer>
        $<$<AND:$<CONFIG:DEBUG>,$<PLATFORM_ID:Linux>>:-fstack-clash-protection>
    )
    # add_link_options(
    #     $<$<CONFIG:DEBUG>:-fsanitize=${sanitizers}>
    # )
endif()

if (ANYLIBS_ENABLE_TESTING)
    message("-- testing has been enabled")
    enable_testing()
    add_subdirectory(test)
endif()

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE include)
add_subdirectory(src)

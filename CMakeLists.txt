include("options.cmake")
include("deps.cmake")

cmake_minimum_required(VERSION 3.20)
project(anylibs LANGUAGES C)

# compiler options
set(CMAKE_C_STANDARD 99)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
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
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Werror
        $<$<CONFIG:RELEASE>:-D_FORTIFY_SOURCE=2>
        $<$<CONFIG:DEBUG>:-fsanitize=address,undefined,leak>
        $<$<CONFIG:DEBUG>:-fstack-protector-strong>
        $<$<CONFIG:DEBUG>:--param=ssp-buffer-size=4>
        $<$<CONFIG:DEBUG>:-fno-omit-frame-pointer>
        $<$<AND:$<CONFIG:DEBUG>,$<PLATFORM_ID:Linux>>:-fstack-clash-protection>
    )
    add_link_options($<$<CONFIG:DEBUG>:-fsanitize=address,undefined,leak>)
endif()

if (${ENABLE_TESTING})
    enable_testing()
    add_subdirectory(test)
endif()

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE include)
add_subdirectory(src)

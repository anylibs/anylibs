include(FetchContent)
include(ExternalProject)

# utest
set(utest_path ${CMAKE_BINARY_DIR}/_deps/utest)
if(NOT EXISTS ${utest_path}/utest.h)
    file(DOWNLOAD https://raw.githubusercontent.com/sheredom/utest.h/master/utest.h ${utest_path}/utest.h)
    file(WRITE ${utest_path}/utest.c 
        "#include \"utest.h\"\n"
        "UTEST_MAIN()\n"
    )
endif()
add_library(utest ${utest_path}/utest.h ${utest_path}/utest.c)
target_include_directories(utest PUBLIC ${utest_path})
add_library(c::utest ALIAS utest)


# jemalloc
# if(NOT WIN32)
#     FetchContent_Declare(
#         jemalloc
#         GIT_REPOSITORY https://github.com/jemalloc/jemalloc.git
#         GIT_TAG 5.3.0  # Specify the version you want to use
#     )
#     # Set where to build the library
#     set(JEMALLOC_INSTALL_DIR ${CMAKE_BINARY_DIR}/jemalloc)
#     FetchContent_MakeAvailable(jemalloc)
#     # Options specific to jemalloc for both platforms
#     set(JEMALLOC_BUILD_SHARED OFF CACHE BOOL "Build jemalloc as a shared library")
#     set(JEMALLOC_WITH_STATIC_PAGE_SHIFT ON CACHE BOOL "Enable static page shift")
#     include(ExternalProject)
#     ExternalProject_Add(jemalloc_project
#         SOURCE_DIR ${jemalloc_SOURCE_DIR}
#         CONFIGURE_COMMAND ${jemalloc_SOURCE_DIR}/autogen.sh --prefix ${JEMALLOC_INSTALL_DIR}
#         BUILD_IN_SOURCE 1
#         INSTALL_DIR ${JEMALLOC_INSTALL_DIR}
#         BUILD_COMMAND make
#         INSTALL_COMMAND make install
#     )
#     # Create a target to link against jemalloc
#     add_library(jemalloc INTERFACE)
#     target_include_directories(jemalloc INTERFACE ${JEMALLOC_INSTALL_DIR}/include)
#     target_link_directories(jemalloc INTERFACE ${JEMALLOC_INSTALL_DIR}/lib)
#     target_link_libraries(jemalloc INTERFACE jemalloc_pic)
# else()
#     add_library(jemalloc INTERFACE)
# endif()

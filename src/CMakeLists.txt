project(anylibs_src LANGUAGES C)

add_library(${PROJECT_NAME}
    vec.c
    error.c
    allocator.c
    str.c
    iter.c
    dl_loader.c
    fs.c
    map.c
)
target_link_libraries(${PROJECT_NAME} anylibs)
if(ANYLIBS_ENABLE_ERROR_CALLBACK)
    message("-- error callback is ON")
    target_compile_definitions(${PROJECT_NAME} PUBLIC ANYLIBS_ENABLE_ERROR_CALLBACK)
endif()

project(anylibs_src LANGUAGES C)

add_library(${PROJECT_NAME}
    vec.c
    error.c
    allocator.c
    str.c
    iter.c
    dl_loader.c
    # fs.c
)
target_link_libraries(${PROJECT_NAME} anylibs)
if(ENABLE_ERROR_CALLBACK)
    message("-- Error callback is ON")
    target_compile_definitions(${PROJECT_NAME} PRIVATE ENABLE_ERROR_CALLBACK)
endif()

project(anylibs_src LANGUAGES C)

add_library(${PROJECT_NAME}
    array.c
    error.c
    allocator.c
)
target_link_libraries(${PROJECT_NAME} anylibs)

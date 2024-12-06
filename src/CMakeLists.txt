project(anylibs_src LANGUAGES C)

add_library(${PROJECT_NAME}
    vec.c
    error.c
    allocator.c
)
target_link_libraries(${PROJECT_NAME} anylibs)

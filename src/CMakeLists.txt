project(anylibs_src LANGUAGES C)

add_library(${PROJECT_NAME}
    array.c
    error.c
)
target_link_libraries(${PROJECT_NAME} anylibs)

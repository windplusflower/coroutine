add_definitions(-DLOG_USE_COLOR)
set_source_files_properties(context.S PROPERTIES LANGUAGE C)

file(GLOB SRC_FILES
    "*.c"
    "*.S"
)

add_library(src STATIC ${SRC_FILES})

target_include_directories(src PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

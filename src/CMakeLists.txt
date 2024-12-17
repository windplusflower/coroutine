# 启用日志颜色
add_definitions(-DLOG_USE_COLOR)
# 启用__USE_GNU宏，用于dlysm
add_definitions(-D_GNU_SOURCE)
# 启用_REENTRANT宏
add_definitions(-D_REENTRANT)
set_source_files_properties(context.S PROPERTIES LANGUAGE C)

file(GLOB SRC_FILES
    "*.c"
    "*.S"
)


add_library(src SHARED ${SRC_FILES})

target_link_libraries(src PUBLIC dl)
target_include_directories(src PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

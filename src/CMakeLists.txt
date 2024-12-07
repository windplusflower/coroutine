add_definitions(-DLOG_USE_COLOR)
set_source_files_properties(context.S PROPERTIES LANGUAGE C)

set(SRC_FILES
    coroutine.c
    event_manager.c
    utils.c
    log.c
    context.c
    context.S
)

add_library(src STATIC ${SRC_FILES})

target_include_directories(src PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

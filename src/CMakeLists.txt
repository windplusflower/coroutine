set(SRC_FILES
    coroutine.c
    epoll_manager.c
    utils.c
)

add_library(src STATIC ${SRC_FILES})

target_include_directories(src PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

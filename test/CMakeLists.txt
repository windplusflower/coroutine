set(TEST_FILES
    test.c
)

add_executable(test ${TEST_FILES})

target_link_libraries(test src)

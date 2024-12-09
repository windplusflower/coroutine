file(GLOB TEST_SOURCES "*.c")

foreach(SOURCE_FILE ${TEST_SOURCES})
    get_filename_component(EXE_NAME ${SOURCE_FILE} NAME_WE)
    add_executable(${EXE_NAME} ${SOURCE_FILE})
    target_link_libraries(${EXE_NAME} src)
endforeach()


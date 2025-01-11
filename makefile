# LOG_LEVEL指定日志级别
# make test_testname可以直接运行对应的test，比如make test_base

LOG_LEVEL ?= LOG_INFO
C_FILES = $(wildcard ./test/*.c)
C_BASENAMES = $(basename $(notdir $(C_FILES)))
TEST_PATH=./build/test

build:
	@mkdir -p build
	@cd build&&cmake ..&&make

$(C_BASENAMES): build
	@LOG_LEVEL=$(LOG_LEVEL) ./build/test/$@

clean:
	@rm -r build

speed: build
	@test -f measure || gcc measure.c -o measure
	@./cmp_speed.sh ${TEST_PATH}/thread_fib ${TEST_PATH}/coroutine_fib 

.PHONY: run build clean debug  $(C_BASENAMES) speed
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

# speed: build
# 	@test -f measure || gcc measure.c -o measure
# 	@./cmp_speed.sh ${TEST_PATH}/thread_fib ${TEST_PATH}/coroutine_fib 
speed: build
	@test -f measure || gcc measure.c -o measure
	@echo "\n"
	@for thread in $(shell find ${TEST_PATH} -maxdepth 1 -type f -name 'thread_*' -executable); do \
		thread_name=$$(basename $$thread); \
		coroutine_name=$$(echo $$thread_name | sed 's/thread_/coroutine_/'); \
		coroutine=$$(dirname $$thread)/$$coroutine_name; \
		echo "Thread: $$thread, Coroutine: $$coroutine"; \
		if [ -f $$coroutine ] && [ -x $$coroutine ]; then \
			echo "Comparing $$thread with $$coroutine"; \
			./cmp_speed.sh $$thread $$coroutine; \
			echo "\n"; \
		fi \
	done





.PHONY: run build clean debug  $(C_BASENAMES) speed
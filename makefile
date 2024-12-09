# LOG_LEVEL指定日志级别
# make test_testname可以直接运行对应的test，比如make test_base

LOG_LEVEL ?= LOG_INFO
C_FILES = $(wildcard ./test/*.c)
C_BASENAMES = $(basename $(notdir $(C_FILES)))

build:
	@mkdir -p build
	@cd build&&cmake ..&&make

$(C_BASENAMES): build
	@LOG_LEVEL=$(LOG_LEVEL) ./build/test/$@

debug: build
	@LOG_LEVEL=LOG_TRACE ./build/test/test

clean:
	@rm -r build

.PHONY: run build clean debug  $(C_BASENAMES) 
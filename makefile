LOG_LEVEL ?= LOG_INFO

build:
	@mkdir -p build
	@cd build&&cmake ..&&make

run: build
	@LOG_LEVEL=$(LOG_LEVEL) ./build/test/test

debug: build
	@LOG_LEVEL=LOG_TRACE ./build/test/test

clean:
	@rm -r build

.PHONY: run build clean
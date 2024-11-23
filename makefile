build:
	-@mkdir build
	@cd build&&cmake ..&&make

run: build
	@./build/test/test

clean:
	@rm -r build

.PHONY: run build clean
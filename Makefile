shell = /bin/sh
.phony: r
r: 
	@if [ -d "build/release" ]; then \
		echo "Removing existing build directory..."; \
		rm -rf build/release; \
	fi

	@if [ -L "./ImageProcessing_Release" ]; then \
		echo "Removing existing symlink..."; \
		rm ImageProcessing_Release; \
	fi

	mkdir -p build/release
	cd build/release && cmake -DCMAKE_BUILD_TYPE=Release ../..
	cd build/release && make

	ln -s ./build/ImageProcessing ./ImageProcessing_Release

.phony: d
d: 
	@if [ -d "build/debug" ]; then \
		echo "Removing existing debug build directory..."; \
		rm -rf build/debug; \
	fi

	@if [ -L "./ImageProcessing_Debug" ]; then \
		echo "Removing existing symlink..."; \
		rm ImageProcessing_Debug; \
	fi

	mkdir -p build/debug
	cd build/debug && cmake ../.. -D CMAKE_BUILD_TYPE=Debug
	cd build/debug && make

	ln -s ./build/debug/ImageProcessing ./ImageProcessing_Debug
.phony: go
go: 
	@if [ -d "build" ]; then \
		echo "Removing existing build directory..."; \
		rm -rf build; \
	fi

	@if [ -L "./ImageProcessing" ]; then \
		echo "Removing existing symlink..."; \
		rm ImageProcessing; \
	fi

	mkdir build
	cd build && cmake ..
	cd build && make

	ln -s ./build/ImageProcessing ./ImageProcessing

	./ImageProcessing ./images/test.png 10

.phony: clean 
clean:
	rm -rf ./build
	rm ./ImageProcessing
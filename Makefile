shell = /bin/sh
.phony: r
r: 
	mkdir -p build/release
	cd build/release && cmake -DCMAKE_BUILD_TYPE=Release ../..
	cd build/release && make

	@if ! [ -L "./ImageProcessing_Release" ]; then \
		ln -s ./build/release/ImageProcessing ./ImageProcessing_Release; \
	fi

.phony: d
d: 
	mkdir -p build/debug
	cd build/debug && cmake ../.. -D CMAKE_BUILD_TYPE=Debug
	cd build/debug && make

	ln -s ./build/debug/ImageProcessing ./ImageProcessing_Debug

.phony: go
go: 
	# @if [ -d "build" ]; then \
	# 	echo "Removing existing build directory..."; \
	# 	rm -rf build; \
	# fi

	# @if [ -L "./ImageProcessing" ]; then \
	# 	echo "Removing existing symlink..."; \
	# 	rm ImageProcessing; \
	# fi

	mkdir build
	cd build && cmake ..
	cd build && make

	ln -s ./build/ImageProcessing ./ImageProcessing

	./ImageProcessing ./images/test.png 10

.phony: clean 
clean:
	rm -rf ./build
	rm ./ImageProcessing

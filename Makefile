shell = /bin/sh
.phony: b
b: 
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

.phony: r
r: 
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
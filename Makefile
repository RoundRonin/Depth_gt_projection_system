shell = /bin/sh
.phony: build
build: 
	if [ ! -d "~/build" ]; then \
      	mkdir build;  \
    fi
	cd build && cmake ..
	cd build && make
	ln -s ./build/ImageProcessing ./ImageProcessing

.phony: clean 
clean:
	rm -rf ./build
	rm ./ImageProcessing

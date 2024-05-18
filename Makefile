shell = /bin/sh

.phony: full_r 
full_r: 
	mkdir -p build/release
	cd build/release && cmake -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release ../..
	cd build/release && make

	@if ! [ -L "./ImageProcessing_Release" ]; then \
		ln -s ./build/release/ImageProcessing ./ImageProcessing_Release; \
	fi

.phony: full_d
full_d: 
	mkdir -p build/debug
	cd build/debug && cmake -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Debug ../..
	cd build/debug && make

	ln -s ./build/debug/ImageProcessing ./ImageProcessing_Debug

.phony: r
r: 
	@if ! [ -d "build/release" ]; then \
		echo "\n\nRelease makefiles not found, running cmake...\n\n"; \
		mkdir -p build/release; \
		cd build/release && cmake -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release ../..; \
	fi

	cd build/release && make

	@if ! [ -L "./ImageProcessing_Release" ]; then \
		ln -s ./build/ImageProcessing ./ImageProcessing; \
	fi

.phony: d
d: 
	@if ! [ -d "build/debug" ]; then \
		echo "\n\nDebug makefiles not found, running cmake...\n\n"; \
		mkdir -p build/debug; \
		cd build/debug && cmake -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Debug ../..; \
	fi

	cd build/debug && make

	@if ! [ -L "./ImageProcessing_Release" ]; then \
		ln -s ./build/ImageProcessing ./ImageProcessing; \
	fi

.phony: go_i
go_i: 
	./ImageProcessing_Release ./images/modified_image.png -ltd 2 -Z 10 -A 1000 -O 15 -D 30 -M 20

.phony: go_svo
go_svo: 
	./ImageProcessing_Release /media/jetson42/E/svo/HD1080_SN39946427_12-05-25.svo -ltd 2 -Z 10 -A 1000 -O 15 -D 30 -M 20

.phony: clean 
clean:
	rm -rf ./build
	rm ./ImageProcessing

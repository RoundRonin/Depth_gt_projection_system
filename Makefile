shell = /bin/sh

.phony: full_r 
full_r: 
	mkdir -p m_build/release
	cd m_build/release && cmake -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release ../..
	cd m_build/release && make

	@if ! [ -L "./ImageProcessing_Release" ]; then \
		ln -s ./m_build/release/ImageProcessing ./ImageProcessing_Release; \
	fi

.phony: full_d
full_d: 
	mkdir -p m_build/debug
	cd m_build/debug && cmake -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=RelWithDebInfo ../..
	cd m_build/debug && make

	ln -s ./m_build/debug/ImageProcessing ./ImageProcessing_Debug

.phony: r
r: 
	@if ! [ -d "m_build/release" ]; then \
		echo "\n\nRelease makefiles not found, running cmake...\n\n"; \
		mkdir -p m_build/release; \
		cd m_build/release && cmake -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release ../..; \
	fi

	cd m_build/release && make

	@if ! [ -L "./ImageProcessing_Release" ]; then \
		ln -s ./m_build/release/ImageProcessing ./ImageProcessing_Release; \
	fi

.phony: d
d: 
	@if ! [ -d "m_build/debug" ]; then \
		echo "\n\nDebug makefiles not found, running cmake...\n\n"; \
		mkdir -p m_build/debug; \
		cd m_build/debug && cmake -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=RelWithDebInfo ../..; \
	fi

	cd m_build/debug && make

	@if ! [ -L "./ImageProcessing_Debug" ]; then \
		ln -s ./m_build/debug/ImageProcessing ./ImageProcessing_Debug; \
	fi

.phony: go_i
go_i: 
	./ImageProcessing_Release ./images/modified_image.png --brief -lt -Z 10 -A 16000 -B 15 -D 30 -M 20

.phony: go_svo
go_svo: 
	./ImageProcessing_Release /media/jetson42/E/svo/HD1080_SN39946427_12-05-25.svo --brief -lt -Z 10 -A 16000 -B 15 -D 30 -M 20

.phony: go_stream
go_stream: 
	./ImageProcessing_Release --brief -lt -Z 10 -A 16000 -B 15 -D 30 -M 20 -T 50 -U 3

.phony: clean 
clean:
	rm -rf ./m_build
	rm ./ImageProcessing_Debug
	rm ./ImageProcessing_Release

.phony: rm_res
rm_res:
	cd Result && rm *
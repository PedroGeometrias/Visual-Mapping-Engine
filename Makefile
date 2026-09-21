EIGEN_INCLUDE ?= /usr/include/eigen3

all: wasm

wasm:
	EIGEN_INCLUDE="$(EIGEN_INCLUDE)" ./scripts/build_wasm.sh

test:
	mkdir -p build
	g++ -std=c++17 -Wall -Wextra -g \
		-Iinclude -isystem "$(EIGEN_INCLUDE)" \
		tests/geometry_test.cpp \
		src/geometry/homography.cpp \
		src/matching/ransac.cpp \
		-o build/geometry_tests
	./build/geometry_tests

serve:
	python3 -m http.server 8000 --directory web

clean:
	rm -rf build web/engine.js web/engine.wasm

.PHONY: all wasm test serve clean

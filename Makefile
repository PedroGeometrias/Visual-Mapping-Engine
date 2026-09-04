CXX = g++
CPPFLAGS = -Iinclude -isystem external/stb
CXXFLAGS = -std=c++17 -Wall -Wextra -g

TARGET = main
SOURCES := $(shell find src -name '*.cpp')
STB_HEADERS = external/stb/stb_image.h external/stb/stb_image_write.h
HEADERS = include/image/image.hpp include/image/image_io.hpp $(STB_HEADERS)

all: $(TARGET)

setup:
	./scripts/fetch_stb.sh

$(TARGET): $(SOURCES) $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(SOURCES) -o $(TARGET)

$(STB_HEADERS):
	@echo "Missing STB headers. Run: make setup"
	@false

clean:
	rm -f $(TARGET)

.PHONY: all setup clean

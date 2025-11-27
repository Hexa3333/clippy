CXX := g++
CXXFLAGS := -std=c++17 -Wextra -Og -g
LDFLAGS := -lX11

SRC := $(wildcard *.cpp)
OBJ := $(patsubst %.cpp,build/%.o,$(SRC))
TARGET := clippy

$(TARGET): $(OBJ)
	$(CXX) $(OBJ) -o $@ $(LDFLAGS)

build/%.o: %.cpp | build
	$(CXX) $(CXXFLAGS) -c $< -o $@

build:
	mkdir -p build

clean:
	rm -rf build $(TARGET)


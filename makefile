# Define variables for compiler and flags
CXX = g++
CXXFLAGS = -Wall -std=c++20
LDFLAGS =

# Define source files and output binary name
SRC = $(wildcard fc.json/*.cpp)
BIN = fc.json

# macOS universal binary configuration
MACOS_ARCHS = x86_64 arm64

# Windows build using MinGW
CXX_WIN = x86_64-w64-mingw32-g++

# Default target (builds all platforms)
# all: macos linux windows
all: macos linux

# macOS universal binary target
macos:
	@echo "Building for macOS (universal binary)..."
	for arch in $(MACOS_ARCHS); do \
		echo "Building for $$arch"; \
		$(CXX) $(CXXFLAGS) -target $$arch-apple-macos11 $(SRC) -o $(BIN)_macos_$$arch; \
	done
	lipo -create -output $(BIN)_macos_universal $(BIN)_macos_x86_64 $(BIN)_macos_arm64
	@echo "Built macOS universal binary: $(BIN)_macos_universal"

# Linux target
linux:
	@echo "Building for Linux..."
	$(CXX) $(CXXFLAGS) $(SRC) -o $(BIN)_linux
	@echo "Built Linux binary: $(BIN)_linux"

# Windows target (using MinGW-w64)
windows:
	@echo "Building for Windows..."
	$(CXX_WIN) $(CXXFLAGS) $(SRC) -o $(BIN)_windows.exe
	@echo "Built Windows binary: $(BIN)_windows.exe"

# Clean all build artifacts
clean:
	rm -f $(BIN)_macos_x86_64 $(BIN)_macos_arm64 $(BIN)_macos_universal
	rm -f $(BIN)_linux
	rm -f $(BIN)_windows.exe
	@echo "Cleaned up all binaries."

# Phony targets
.PHONY: all macos linux windows clean


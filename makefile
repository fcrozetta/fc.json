# Define variables for compiler and flags
CXX = g++
CXXFLAGS = -Wall -std=c++20
LDFLAGS =

# Define source files and output binary name
SRC = $(wildcard fc.json/*.cpp)
OUTPUT_DIR = output
BIN = fc.json

# macOS universal binary configuration
MACOS_ARCHS = x86_64 arm64

# Windows build using MinGW
CXX_WIN = x86_64-w64-mingw32-g++



# Default target (builds all platforms)
# all: macos linux windows
all: macos linux

$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)

# macOS universal binary target
macos: $(OUTPUT_DIR)
	@echo "Building for macOS (universal binary)..."
	for arch in $(MACOS_ARCHS); do \
		echo "Building for $$arch"; \
		$(CXX) $(CXXFLAGS) -target $$arch-apple-macos11 $(SRC) -o $(OUTPUT_DIR)/$(BIN)_macos_$$arch; \
	done
	lipo -create -output $(OUTPUT_DIR)/$(BIN) $(OUTPUT_DIR)/$(BIN)_macos_x86_64 $(OUTPUT_DIR)/$(BIN)_macos_arm64
	@echo "Built macOS universal binary: $(OUTPUT_DIR)/$(BIN)"

# Linux target
linux:
	@echo "Building for Linux..."
	$(CXX) $(CXXFLAGS) $(SRC) -o $(OUTPUT_DIR)/$(BIN)_linux
	@echo "Built Linux binary: $(OUTPUT_DIR)/$(BIN)_linux"

# Windows target (using MinGW-w64)
windows: $(OUTPUT_DIR)
	@echo "Building for Windows..."
	$(CXX_WIN) $(CXXFLAGS) $(SRC) -o $(OUTPUT_DIR)/$(BIN)_windows.exe
	@echo "Built Windows binary: $(OUTPUT_DIR)/$(BIN)_windows.exe"

# Clean all build artifacts
clean:
	rm -rf output/
	@echo "Cleaned up all binaries."
# Phony targets
.PHONY: all macos linux windows clean


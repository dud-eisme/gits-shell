# Compiler configuration
CXX      := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -Isrc

# Target executable name
TARGET   := gits

# Directories
SRC_DIR  := src
BUILD_DIR:= build

# Source and Object file discovery
# Finds all .cpp files in src/ and maps them to build/*.o
SRCS     := $(wildcard $(SRC_DIR)/*.cpp)
OBJS     := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))

# Default target: builds the final shell executable
all: $(TARGET)

# Link phase: combines all .o files into the final binary
$(TARGET): $(OBJS)
	@echo "Linking object files into executable: $(TARGET)..."
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(TARGET)
	@echo "Build complete! Run ./\$(TARGET) to launch."

# Compilation phase: builds individual .o files from .cpp files
# The '| $(BUILD_DIR)' is an order-only prerequisite ensuring the folder exists first
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	@echo "Compiling $< -> $@"
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Folder creation rule: builds the obj directory if missing
$(BUILD_DIR):
	@echo "Creating build directory..."
	mkdir -p $(BUILD_DIR)

# Clean rule: wipes out generated artifacts to force a fresh rebuild
clean:
	@echo "Cleaning up build artifacts..."
	rm -rf $(BUILD_DIR) $(TARGET)

# Declaring targets that aren't physical files to avoid naming conflicts
.PHONY: all clean

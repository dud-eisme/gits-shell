# Compiler configuration
CXX      := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -Iinclude -MMD -MP
LDLIBS   := -lgit2

# Target executable name
TARGET   := gits

# Directories
SRC_DIR  := src
INC_DIR  := include
BUILD_DIR:= build

# Source and Object file discovery
# Finds all .cpp files in src/ and maps them to build/*.o
SRCS     := $(wildcard $(SRC_DIR)/*.cpp)
OBJS     := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))
DEPS     := $(OBJS:.o=.d)

# Default target: builds the final shell executable
all: $(TARGET)

# Link phase: combines all .o files into the final binary.
# LDLIBS comes after $(OBJS) so the linker sees the object files'
# undefined symbols before it searches -lgit2 for them.
$(TARGET): $(OBJS)
	@echo "Linking object files into executable: $(TARGET)..."
	$(CXX) $(OBJS) -o $(TARGET) $(LDLIBS)
	@echo "Build complete! Run ./$(TARGET) to launch."

# Compilation phase: builds individual .o files from .cpp files
# The '| $(BUILD_DIR)' is an order-only prerequisite ensuring the folder exists first
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	@echo "Compiling $< -> $@"
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Folder creation rule: builds the obj directory if missing
$(BUILD_DIR):
	@echo "Creating build directory..."
	mkdir -p $(BUILD_DIR)

# Pull in auto-generated header dependencies (from -MMD -MP above) so
# editing a .hpp correctly triggers a rebuild of anything that includes it.
-include $(DEPS)

# Clean rule: wipes out generated artifacts to force a fresh rebuild
clean:
	@echo "Cleaning up build artifacts..."
	rm -rf $(BUILD_DIR) $(TARGET)

fmt:
	clang-format -i -style="{BasedOnStyle: LLVM, BreakBeforeBraces: Stroustrup}" src/*.cpp include/*.hpp
	@echo "Format Done"

# Declaring targets that aren't physical files to avoid naming conflicts
.PHONY: all clean fmt

# Compiler settings
CC = gcc
CFLAGS_BASE = -Wall -Wextra -std=c11
CFLAGS = $(CFLAGS_BASE) -O2
CFLAGS_DEBUG = $(CFLAGS_BASE) -O0 -g -DDEBUG
INCLUDES = -I./drivers -I./src
LDFLAGS =
LIBS = -lbcm2835

# Directories
SRC_DIR = src
DRIVER_DIR = drivers
ASSETS_DIR = assets
BUILD_DIR = build
BIN_DIR = bin

# Source files
SOURCES = $(SRC_DIR)/main.c \
          $(SRC_DIR)/maps/easy_map.c \
          $(SRC_DIR)/maps/hard_map.c \
          $(DRIVER_DIR)/common/gpio_init.c \
          $(DRIVER_DIR)/lcd/st7789.c \
          $(DRIVER_DIR)/lcd/framebuffer.c \
          $(DRIVER_DIR)/input/button.c \
          $(DRIVER_DIR)/input/joystick.c \
          $(DRIVER_DIR)/game/car_physics.c \
          $(DRIVER_DIR)/game/collision.c \
          $(ASSETS_DIR)/car.c \
          $(ASSETS_DIR)/handle.c \
          $(ASSETS_DIR)/easy_map.c \
          $(ASSETS_DIR)/intro.c \
          $(ASSETS_DIR)/hard_map.c \
          $(ASSETS_DIR)/obstacle.c \
          $(ASSETS_DIR)/game_over.c \
          $(ASSETS_DIR)/complete.c

# Object files
OBJECTS = $(SOURCES:%.c=$(BUILD_DIR)/%.o)
OBJECTS_DEBUG = $(SOURCES:%.c=$(BUILD_DIR)/debug/%.o)

# Target executable
TARGET = $(BIN_DIR)/main
TARGET_DEBUG = $(BIN_DIR)/main_debug

# Default target (release)
all: directories $(TARGET)

# Debug build
debug: directories $(TARGET_DEBUG)

# Create necessary directories
directories:
	@mkdir -p $(BUILD_DIR)/$(SRC_DIR)
	@mkdir -p $(BUILD_DIR)/$(SRC_DIR)/maps
	@mkdir -p $(BUILD_DIR)/$(DRIVER_DIR)/common
	@mkdir -p $(BUILD_DIR)/$(DRIVER_DIR)/lcd
	@mkdir -p $(BUILD_DIR)/$(DRIVER_DIR)/input
	@mkdir -p $(BUILD_DIR)/$(DRIVER_DIR)/game
	@mkdir -p $(BUILD_DIR)/$(ASSETS_DIR)
	@mkdir -p $(BIN_DIR)

# Link object files to create executable (release)
$(TARGET): $(OBJECTS) | directories
	@echo "Linking $@..."
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)
	@echo "Build complete: $@"

# Link object files to create executable (debug)
$(TARGET_DEBUG): $(OBJECTS_DEBUG) | directories
	@echo "Linking $@ (DEBUG)..."
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)
	@echo "Debug build complete: $@"

# Compile source files to object files (release)
$(BUILD_DIR)/%.o: %.c
	@echo "Compiling $<..."
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Compile source files to object files (debug)
$(BUILD_DIR)/debug/%.o: %.c
	@echo "Compiling $< (DEBUG)..."
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_DEBUG) $(INCLUDES) -c $< -o $@

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(BUILD_DIR) $(BIN_DIR)
	@echo "Clean complete"

# Run the program (release, requires sudo for BCM2835)
run: $(TARGET)
	@echo "Running $(TARGET)..."
	sudo $(TARGET)

# Run the program (debug, requires sudo for BCM2835)
run-debug: $(TARGET_DEBUG)
	@echo "Running $(TARGET_DEBUG) (DEBUG mode)..."
	sudo $(TARGET_DEBUG)

# Install bcm2835 library (run once)
install-bcm2835:
	@echo "Installing BCM2835 library..."
	cd lib/bcm2835-1.75 && ./configure && make && sudo make install
	@echo "BCM2835 library installed"

# Help target
help:
	@echo "Available targets:"
	@echo "  all              - Build release version (default)"
	@echo "  debug            - Build debug version (with hitbox outlines)"
	@echo "  clean            - Remove build artifacts"
	@echo "  run              - Build and run release version"
	@echo "  run-debug        - Build and run debug version"
	@echo "  install-bcm2835  - Install BCM2835 library system-wide"
	@echo "  help             - Show this help message"

.PHONY: all debug clean run run-debug install-bcm2835 help directories


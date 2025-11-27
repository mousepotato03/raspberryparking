# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11
INCLUDES = -I./drivers
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
          $(DRIVER_DIR)/common/gpio_init.c \
          $(DRIVER_DIR)/lcd/st7789.c \
          $(DRIVER_DIR)/lcd/framebuffer.c \
          $(DRIVER_DIR)/input/button.c \
          $(DRIVER_DIR)/input/joystick.c \
          $(DRIVER_DIR)/game/car_physics.c \
          $(ASSETS_DIR)/images.c \
          $(ASSETS_DIR)/car.c \
          $(ASSETS_DIR)/handle.c \
          $(ASSETS_DIR)/easy_map.c

# Object files
OBJECTS = $(SOURCES:%.c=$(BUILD_DIR)/%.o)

# Target executable
TARGET = $(BIN_DIR)/main

# Default target
all: directories $(TARGET)

# Create necessary directories
directories:
	@mkdir -p $(BUILD_DIR)/$(SRC_DIR)
	@mkdir -p $(BUILD_DIR)/$(DRIVER_DIR)/common
	@mkdir -p $(BUILD_DIR)/$(DRIVER_DIR)/lcd
	@mkdir -p $(BUILD_DIR)/$(DRIVER_DIR)/input
	@mkdir -p $(BUILD_DIR)/$(DRIVER_DIR)/game
	@mkdir -p $(BUILD_DIR)/$(ASSETS_DIR)
	@mkdir -p $(BIN_DIR)

# Link object files to create executable
$(TARGET): $(OBJECTS) | directories
	@echo "Linking $@..."
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)
	@echo "Build complete: $@"

# Compile source files to object files
$(BUILD_DIR)/%.o: %.c
	@echo "Compiling $<..."
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(BUILD_DIR) $(BIN_DIR)
	@echo "Clean complete"

# Run the program (requires sudo for BCM2835)
run: $(TARGET)
	@echo "Running $(TARGET)..."
	sudo $(TARGET)

# Install bcm2835 library (run once)
install-bcm2835:
	@echo "Installing BCM2835 library..."
	cd lib/bcm2835-1.75 && ./configure && make && sudo make install
	@echo "BCM2835 library installed"

# Help target
help:
	@echo "Available targets:"
	@echo "  all              - Build the project (default)"
	@echo "  clean            - Remove build artifacts"
	@echo "  run              - Build and run the program (requires sudo)"
	@echo "  install-bcm2835  - Install BCM2835 library system-wide"
	@echo "  help             - Show this help message"

.PHONY: all clean run install-bcm2835 help directories


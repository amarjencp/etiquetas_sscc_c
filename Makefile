CC = gcc
CFLAGS = -Wall -O3 -march=native -mtune=native
LDFLAGS = -lzint -lhpdf -lpng -lz

SRCS = etiquetas_sscc.c
TARGET = etiquetas
BUILD_DIR = build
BIN_DIR = /usr/local/bin

# Ensure the build directory exists before building
all: $(BUILD_DIR)/$(TARGET)

# Save the executable in the build directory
$(BUILD_DIR)/$(TARGET): $(SRCS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $(BUILD_DIR)/$(TARGET) $(SRCS) $(LDFLAGS)

clean:
	rm -f $(BUILD_DIR)/$(TARGET)
	rm -f ./output/*.pdf

run: $(BUILD_DIR)/$(TARGET)
	./$(BUILD_DIR)/$(TARGET) $(ARGS)

# Install the target in the specified bin directory and set permissions
install: $(BUILD_DIR)/$(TARGET)
	@mkdir -p $(BIN_DIR)
	install $(BUILD_DIR)/$(TARGET) $(BIN_DIR)

# Uninstall the target from the bin directory
uninstall:
	rm -f $(BIN_DIR)/$(TARGET)

.PHONY: all clean run install uninstall

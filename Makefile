# Compiler and source file
CC = gcc
SRC = ncurses-menu.c

# Output file names
TARGET = ncurses-menu
TARGET_STATIC = ncurses-menu-static
TARGET_SMALL = ncurses-menu-small
TARGET_PACKED = ncurses-menu-packed

# Flags for dynamic build
LDFLAGS = -lncurses -ltinfo

# Flags for size optimization
CFLAGS_SIZE = -Os -flto

# --- Build targets ---

# Default target: build dynamic binary
all: $(TARGET)

# Rule for building dynamic binary
$(TARGET): $(SRC)
	$(CC) $(SRC) $(LDFLAGS) -o $(TARGET)

# 1. Build regular static binary
static: $(SRC)
	@echo "Building regular static binary..."
	$(CC) -static $(SRC) $(LDFLAGS) -o $(TARGET_STATIC)

# 2. Build optimized and stripped static binary
static-small: $(SRC)
	@echo "Building size-optimized binary..."
	$(CC) -static $(CFLAGS_SIZE) $(SRC) $(LDFLAGS) -o $(TARGET_SMALL)
	@echo "Stripping debug information..."
	strip $(TARGET_SMALL)
	@echo "Created binary: $(TARGET_SMALL)"

# 3. Build compressed (UPX) binary
static-packed: static-small
	@echo "Compressing binary with UPX..."
	cp $(TARGET_SMALL) $(TARGET_PACKED)
	upx --best --lzma $(TARGET_PACKED)
	@echo "Created compressed binary: $(TARGET_PACKED)"

# 4. Test the ncurses-menu binary
test: $(TARGET)
	@echo "Running test..."
	TERM=xfce ./$(TARGET) -t "Test title which is very longggggggggggggggggggggggggggggggggggggggggggggggggggggg\nTest second line which is very longggggggggggggggggggggggggggggggggggggggggggggggggggggg" -o "1 aaa" -o "2 sss" -o "3 ddd" -o "select this" -o "4 fff" 2> out.txt
	@if [ "$$(cat out.txt)" != "select this" ]; then \
		echo "!!!!! TEST ERROR !!!!!"; \
		rm -f out.txt; \
		exit 1; \
	fi
	@echo "Test passed successfully."
	@rm -f out.txt

.PHONY: all static static-small static-packed test clean

clean:
	@echo "Cleaning up working files..."
	rm -f $(TARGET) $(TARGET_STATIC) $(TARGET_SMALL) $(TARGET_PACKED) out.txt

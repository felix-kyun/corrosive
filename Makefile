# Makefile
CC := clang
CFLAGS := -Wall -Wextra -Werror -pedantic -std=c23 \
	-Wshadow -Wconversion -Wnull-dereference -Wformat=2 -Wundef
DEBUG_FLAGS := -g -fsanitize=address,undefined,leak -O0
LDFLAGS :=
RELEASE_FLAGS := -O3 -flto

TARGET := huffman
LIBS :=
INCLUDE_DIR := include
SOURCE_DIR := src
BUILD_DIR := build
TEST_DIR := tests

# do not modify
INCLUDES := -I$(INCLUDE_DIR)
SOURCES := $(shell find $(SOURCE_DIR) -type f -name '*.c')
OBJS := $(patsubst $(SOURCE_DIR)/%.c, $(BUILD_DIR)/%.o, $(SOURCES))

.PHONY: all
all: CFLAGS += $(RELEASE_FLAGS)
all: $(TARGET)

.PHONY: debug
debug: CFLAGS += $(DEBUG_FLAGS)
debug: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LIBS) $(LDFLAGS) $(CFLAGS)

$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.c | $(BUILD_DIR)
	$(CC) -c $< -o $@ $(CFLAGS) $(INCLUDES)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

test-%: CFLAGS += $(DEBUG_FLAGS)
test-%: $(SOURCE_DIR)/%.c $(TEST_DIR)/%.c $(TEST_DEPS_%) | $(BUILD_DIR)
	$(CC) -o $(BUILD_DIR)/test_$* $^ $(INCLUDES) $(CFLAGS)
	./$(BUILD_DIR)/test_$*

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

setup:
	@mkdir -p $(BUILD_DIR) $(SOURCE_DIR) $(INCLUDE_DIR) $(TEST_DIR)

dev:
	bear -- $(MAKE) debug

format:
	@fd -ec -eh . -x clang-format -i {}

.PHONY: clean setup dev format test-%

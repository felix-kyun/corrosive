# Makefile
CC := clang
CFLAGS := -Wall -Wextra -Werror -pedantic -std=c23 \
	-Wshadow -Wconversion -Wnull-dereference -Wformat=2 -Wundef \
	-g -fsanitize=address,undefined,leak -O0 \
	-DCORROSIVE_IMPLEMENTATION

BUILD_DIR := build
TEST_DIR := tests

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

format:
	@echo "Formatting code..."
	@fd -ec -eh . -x clang-format -i {}

watch-%:
	@echo "Watching for changes..."
	@fd -ec -eh . | entr -cd $(MAKE) -s $*

test-%: $(TEST_DIR)/%.c | $(BUILD_DIR)
	$(CC) -o $(BUILD_DIR)/$* $^ $(CFLAGS) -I.
	@./$(BUILD_DIR)/$*

.PHONY: format clean test-% watch-%

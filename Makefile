# Makefile
CC := gcc
CFLAGS := -Wall -Wextra -Werror -pedantic
INCLUDE_DIR := include
SOURCE_DIR := src
BUILD_DIR := build
INCLUDES := -I$(INCLUDE_DIR)
LIBS :=
SOURCES := $(shell find $(SOURCE_DIR) -type f -name '*.c')
TARGET := huffman

_TARGET = $(addprefix $(BUILD_DIR)/, $(TARGET))
_OBJS = $(patsubst $(SOURCE_DIR)/%.c, $(BUILD_DIR)/%.o, $(SOURCES))

all: $(_TARGET)
	mv $(_TARGET) .

$(_TARGET): $(_OBJS)
	$(CC) -o $@ $^ $(LIBS) $(INCLUDES) $(CFLAGS) # for c programs
	ar rcs $@ $^ # for libraries (static)

$(_OBJS): $(BUILD_DIR)/%.o: src/%.c
	$(shell [ -d $(dir $@) ] || mkdir -p $(dir $@))
	$(CC) -c $< -o $@ $(CFLAGS) $(INCLUDES)


lazy: # for lazy people
	make clean
	make all
	./$(TARGET)

clean:
	rm -rf build/*
	rm $(TARGET)

setup:
	$(shell [ -d $(BUILD_DIR) ] || mkdir -p $(BUILD_DIR))
	$(shell [ -d $(SOURCE_DIR) ] || mkdir -p $(SOURCE_DIR))
	$(shell [ -d $(INCLUDE_DIR) ] || mkdir -p $(INCLUDE_DIR))

dev:
	bear -- make all

.PHONY: clean all test dev lazy

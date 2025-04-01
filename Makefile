CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -Wformat=2 -Wconversion -fsanitize=address -std=c17
BUILD_DIR = build
SRC_DIR = src
TEST_DIR = test

_SRCS = main.c linked-list.c workers.c sched.c sync.c log.c
SRCS = $(SRCS:%.c=$(SRC_DIR)/%.c)

_OBJS = main.o linked-list.o workers.o sched.o sync.o log.o
OBJS = $(_OBJS:%.o=$(BUILD_DIR)/%.o)

_DEPS = linked-list.h sync.h workers.h sched.h sync.h log.h
DEPS = $(_DEPS:%.h=$(SRC_DIR)/%.h)

EXEC = $(BUILD_DIR)/main

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(BUILD_DIR)/main: $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

run: $(BUILD_DIR)/main
	$(BUILD_DIR)/main

.PHONY: clean test

test:
	$(MAKE) -C $(TEST_DIR) test

clean:
	rm -f $(BUILD_DIR)/*

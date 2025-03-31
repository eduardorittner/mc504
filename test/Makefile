CC = gcc
CFLAGS = -Wall -O2 -Wextra -Wpedantic -Wformat=2 -Wconversion -std=c17 -fsanitize=address
BUILD_DIR = build
SRC_DIR = ../src

_SRCS = linked-list.c workers.c sched.c sync.c
SRCS = $(SRCS:%.c=$(SRC_DIR)/%.c)

TEST_SRCS = test.c

TEST_OBJS = $(TEST_SRCS:%.c=$(BUILD_DIR)/%.o)

TEST_DEPS = greatest.h

_OBJS = linked-list.o workers.o sched.o sync.o
OBJS = $(_OBJS:%.o=$(BUILD_DIR)/%.o)

_DEPS = linked-list.h sync.h workers.h sched.h sync.h
DEPS = $(_DEPS:%.h=$(SRC_DIR)/%.h)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(BUILD_DIR)/%.o: %.c $(DEPS) $(TEST_DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(BUILD_DIR)/test: $(OBJS) $(TEST_OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

test: $(BUILD_DIR)/test
	$(BUILD_DIR)/test


.PHONY: clean

clean:
	rm -f $(BUILD_DIR)/*

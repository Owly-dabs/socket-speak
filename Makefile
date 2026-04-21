CC := gcc
CFLAGS := -ansi -pedantic -Wall -Werror -Wno-newline-eof
LDLIBS := -lpthread

.SILENT:

SRCS := $(wildcard *.c)
OBJ_DIR := build
BIN_DIR := bin

DEBUG_OBJ_DIR := build_debug
DEBUG_BIN_DIR := bin_debug

MAIN_SRC := main.c
GROUP_SRCS := gserver.c gclient.c

COMMON_SRCS := $(filter-out $(MAIN_SRC) $(GROUP_SRCS), $(SRCS))
COMMON_OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(COMMON_SRCS))
MAIN_OBJS := $(OBJ_DIR)/main.o $(COMMON_OBJS)

DEBUG_COMMON_OBJS := $(patsubst %.c,$(DEBUG_OBJ_DIR)/%.o,$(COMMON_SRCS))
DEBUG_MAIN_OBJS := $(DEBUG_OBJ_DIR)/main.o $(DEBUG_COMMON_OBJS)

GROUP_BINS := $(BIN_DIR)/gserver $(BIN_DIR)/gclient
DEBUG_GROUP_BINS := $(DEBUG_BIN_DIR)/gserver $(DEBUG_BIN_DIR)/gclient

TEST_SRCS := $(wildcard test/*.c)
TEST_OBJ_DIR := $(OBJ_DIR)/test
TEST_BINS := $(patsubst test/%.c,$(BIN_DIR)/%,$(TEST_SRCS))

DEBUG_TEST_OBJ_DIR := $(DEBUG_OBJ_DIR)/test
DEBUG_TEST_BINS := $(patsubst test/%.c,$(DEBUG_BIN_DIR)/%,$(TEST_SRCS))

.PHONY: all tests debug debug-tests clean

all: $(BIN_DIR)/main $(GROUP_BINS) tests
	rm -rf $(OBJ_DIR)

tests: $(TEST_BINS)

$(BIN_DIR)/main: $(MAIN_OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ $(LDLIBS) -o $@

$(BIN_DIR)/gserver: $(OBJ_DIR)/gserver.o $(COMMON_OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ $(LDLIBS) -o $@

$(BIN_DIR)/gclient: $(OBJ_DIR)/gclient.o $(COMMON_OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ $(LDLIBS) -o $@

$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I. -c $< -o $@

$(OBJ_DIR):
	mkdir -p $@

$(BIN_DIR):
	mkdir -p $@

$(TEST_OBJ_DIR)/%.o: test/%.c | $(TEST_OBJ_DIR)
	$(CC) $(CFLAGS) -I. -c $< -o $@

$(BIN_DIR)/%: $(TEST_OBJ_DIR)/%.o $(COMMON_OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ $(LDLIBS) -o $@

$(TEST_OBJ_DIR):
	mkdir -p $@

debug: CFLAGS += -g -O0 -DDEBUG
debug: $(DEBUG_BIN_DIR)/main $(DEBUG_GROUP_BINS) debug-tests
	rm -rf $(DEBUG_OBJ_DIR)

debug-tests: $(DEBUG_TEST_BINS)

$(DEBUG_BIN_DIR)/main: $(DEBUG_MAIN_OBJS) | $(DEBUG_BIN_DIR)
	$(CC) $(CFLAGS) $^ $(LDLIBS) -o $@

$(DEBUG_BIN_DIR)/gserver: $(DEBUG_OBJ_DIR)/gserver.o $(DEBUG_COMMON_OBJS) | $(DEBUG_BIN_DIR)
	$(CC) $(CFLAGS) $^ $(LDLIBS) -o $@

$(DEBUG_BIN_DIR)/gclient: $(DEBUG_OBJ_DIR)/gclient.o $(DEBUG_COMMON_OBJS) | $(DEBUG_BIN_DIR)
	$(CC) $(CFLAGS) $^ $(LDLIBS) -o $@

$(DEBUG_OBJ_DIR)/%.o: %.c | $(DEBUG_OBJ_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I. -c $< -o $@

$(DEBUG_OBJ_DIR):
	mkdir -p $@

$(DEBUG_BIN_DIR):
	mkdir -p $@

$(DEBUG_TEST_OBJ_DIR)/%.o: test/%.c | $(DEBUG_TEST_OBJ_DIR)
	$(CC) $(CFLAGS) -I. -c $< -o $@

$(DEBUG_BIN_DIR)/%: $(DEBUG_TEST_OBJ_DIR)/%.o $(DEBUG_COMMON_OBJS) | $(DEBUG_BIN_DIR)
	$(CC) $(CFLAGS) $^ $(LDLIBS) -o $@

$(DEBUG_TEST_OBJ_DIR):
	mkdir -p $@

clean:
	rm -rf $(BIN_DIR)/main $(GROUP_BINS) $(TEST_BINS)
	rm -rf $(DEBUG_OBJ_DIR) $(DEBUG_BIN_DIR)/main $(DEBUG_GROUP_BINS) $(DEBUG_TEST_BINS)
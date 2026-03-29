CC := gcc
CFLAGS := -ansi -pedantic -Wall -Werror
LDLIBS := -lpthread

.SILENT:

SRCS := $(wildcard *.c)
OBJ_DIR := build
BIN_DIR := bin

DEBUG_OBJ_DIR := build_debug
DEBUG_BIN_DIR := bin_debug
MAIN_SRC := main.c
HELPER_MAIN_SRC := localAddr.c test.c scrapys.c scrapyc.c

COMMON_SRCS := $(filter-out $(MAIN_SRC) $(HELPER_MAIN_SRC),$(SRCS))
COMMON_OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(COMMON_SRCS))
MAIN_OBJS := $(OBJ_DIR)/main.o $(COMMON_OBJS)
DEBUG_COMMON_OBJS := $(patsubst %.c,$(DEBUG_OBJ_DIR)/%.o,$(COMMON_SRCS))
DEBUG_MAIN_OBJS := $(DEBUG_OBJ_DIR)/main.o $(DEBUG_COMMON_OBJS)

.PHONY: all debug clean

all: $(BIN_DIR)/server $(BIN_DIR)/client $(BIN_DIR)/main
	rm -rf $(OBJ_DIR)

$(BIN_DIR)/main: $(MAIN_OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ $(LDLIBS) -o $@

$(BIN_DIR)/server: $(BIN_DIR)/main | $(BIN_DIR)
	printf '%s\n' '#!/bin/sh' 'exec "$$(dirname "$$0")/main" -u server "$$@"' > $@
	chmod +x $@

$(BIN_DIR)/client: $(BIN_DIR)/main | $(BIN_DIR)
	printf '%s\n' '#!/bin/sh' 'exec "$$(dirname "$$0")/main" -u client "$$@"' > $@
	chmod +x $@

$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $@

$(BIN_DIR):
	mkdir -p $@

debug: CFLAGS += -g -O0 -DDEBUG
debug: $(DEBUG_BIN_DIR)/server $(DEBUG_BIN_DIR)/client $(DEBUG_BIN_DIR)/main
	rm -rf $(DEBUG_OBJ_DIR)

$(DEBUG_BIN_DIR)/main: $(DEBUG_MAIN_OBJS) | $(DEBUG_BIN_DIR)
	$(CC) $(CFLAGS) $^ $(LDLIBS) -o $@

$(DEBUG_BIN_DIR)/server: $(DEBUG_BIN_DIR)/main | $(DEBUG_BIN_DIR)
	printf '%s\n' '#!/bin/sh' 'exec "$$(dirname "$$0")/main" -u server "$$@"' > $@
	chmod +x $@

$(DEBUG_BIN_DIR)/client: $(DEBUG_BIN_DIR)/main | $(DEBUG_BIN_DIR)
	printf '%s\n' '#!/bin/sh' 'exec "$$(dirname "$$0")/main" -u client "$$@"' > $@
	chmod +x $@

$(DEBUG_OBJ_DIR)/%.o: %.c | $(DEBUG_OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(DEBUG_OBJ_DIR):
	mkdir -p $@

$(DEBUG_BIN_DIR):
	mkdir -p $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)/server $(BIN_DIR)/client $(BIN_DIR)/main
	rm -rf $(DEBUG_OBJ_DIR) $(DEBUG_BIN_DIR)/server $(DEBUG_BIN_DIR)/client $(DEBUG_BIN_DIR)/main
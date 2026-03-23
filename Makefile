CC := gcc
CFLAGS := -ansi -pedantic
LDLIBS := -lpthread

.SILENT:

SRCS := $(wildcard *.c)
OBJ_DIR := build
BIN_DIR := bin

SERVER_SRC := server.c
CLIENT_SRC := client.c
HELPER_MAIN_SRC := localAddr.c test.c

COMMON_SRCS := $(filter-out $(SERVER_SRC) $(CLIENT_SRC) $(HELPER_MAIN_SRC),$(SRCS))
COMMON_OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(COMMON_SRCS))
SERVER_OBJS := $(OBJ_DIR)/server.o $(COMMON_OBJS)
CLIENT_OBJS := $(OBJ_DIR)/client.o $(COMMON_OBJS)

.PHONY: all clean

all: $(BIN_DIR)/server $(BIN_DIR)/client
	rm -rf $(OBJ_DIR)

$(BIN_DIR)/server: $(SERVER_OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ $(LDLIBS) -o $@

$(BIN_DIR)/client: $(CLIENT_OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ $(LDLIBS) -o $@

$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $@

$(BIN_DIR):
	mkdir -p $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)/server $(BIN_DIR)/client

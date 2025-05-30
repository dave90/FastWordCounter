CC = gcc
CFLAGS = -Wall -Wextra -O2
DEBUG_CFLAGS = -Wall -Wextra -g -O0
LDFLAGS = -lpthread

SRC_DIR = src
BUILD_DIR = build
BIN = fwc
BIN_CLI = fwc-cli

$(BUILD_DIR):
	mkdir -p $@

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

FWC_SRCS = $(SRC_DIR)/fwc.c $(SRC_DIR)/commandExecutor.c $(SRC_DIR)/commandParser.c $(SRC_DIR)/dict.c $(SRC_DIR)/log.c $(SRC_DIR)/network.c
FWC_OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(FWC_SRCS))

CLI_SRCS = $(SRC_DIR)/fwc-cli.c $(SRC_DIR)/log.c $(SRC_DIR)/network.c
CLI_OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(CLI_SRCS))

all: $(BIN) $(BIN_CLI)

$(BIN): $(FWC_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BIN_CLI): $(CLI_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Debug target
debug: CFLAGS = $(DEBUG_CFLAGS)
debug: clean $(BIN) $(BIN_CLI)

clean:
	rm -rf $(BUILD_DIR) $(BIN) $(BIN_CLI)

rebuild: clean all

# Run Python tests on debug build
test: debug
	pytest tests

.PHONY: all clean rebuild

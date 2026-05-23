CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -D_GNU_SOURCE -I src -I cli
LDFLAGS = -lpcap -lsqlite3 -lpthread -lm

# Linux
LIBS = -lpcap -lsqlite3 -lpthread -lm

# Windows (Npcap): override with
# LIBS = -lwpcap -lsqlite3 -lpthread -lm -lws2_32

SRC_DIR = src
CLI_DIR = cli
TEST_DIR = tests
BUILD_DIR = build

SRCS = $(wildcard $(SRC_DIR)/*.c)
CLI_SRCS = $(wildcard $(CLI_DIR)/*.c)
DAEMON_SRCS = $(filter-out $(SRC_DIR)/main.c, $(SRCS))

DAEMON_OBJ = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
CLI_OBJ = $(CLI_SRCS:$(CLI_DIR)/%.c=$(BUILD_DIR)/cli_%.o)

.PHONY: all clean test install

all: vigil vigil-cli

vigil: $(DAEMON_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

vigil-cli: $(CLI_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/cli_%.o: $(CLI_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

TEST_SRCS = $(filter-out $(TEST_DIR)/test_main.c $(TEST_DIR)/test_stubs.c,$(wildcard $(TEST_DIR)/test_*.c))

test: all
	$(CC) $(CFLAGS) -o $(BUILD_DIR)/test_runner \
		$(TEST_DIR)/test_main.c \
		$(TEST_DIR)/test_stubs.c \
		$(TEST_SRCS) \
		$(DAEMON_SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o) \
		$(LIBS) -lcheck -lm -lpthread -lrt -lsubunit
	./$(BUILD_DIR)/test_runner

clean:
	rm -rf $(BUILD_DIR) vigil vigil-cli

install:
	cp vigil /usr/local/bin/
	cp vigil-cli /usr/local/bin/
	cp vigil.conf.example /etc/vigil.conf

CC = gcc
CFLAGS = -Wall -Wextra -O2 $(shell pkg-config --cflags gtk+-3.0 libqrencode zbar gstreamer-1.0 gstreamer-app-1.0)
LDFLAGS = $(shell pkg-config --libs gtk+-3.0 libqrencode zbar gstreamer-1.0 gstreamer-app-1.0)

SRC_DIR = src
OBJ_DIR = build
BIN = vqr

SRCS = $(wildcard $(SRC_DIR)/*.c) \
       $(wildcard $(SRC_DIR)/domain/*.c) \
       $(wildcard $(SRC_DIR)/usecases/*.c) \
       $(wildcard $(SRC_DIR)/data/*.c) \
       $(wildcard $(SRC_DIR)/presentation/*.c)

OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(OBJ_DIR) $(BIN)

.PHONY: all clean

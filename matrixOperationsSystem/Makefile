# ====== Build settings ======
CC       := gcc
CFLAGS   := -Wall -Wextra -O2 -std=c17 -g -fopenmp
CFLAGS   += -D_DEFAULT_SOURCE
CPPFLAGS := -Iinclude

SRC_DIR  := src
BUILD    := build
TARGET   := proj1

# auto-discover sources/objects
SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD)/%.o,$(SRCS))

# ====== Targets ======
.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(BUILD) $(OBJS)
	$(CC) $(OBJS) -o $@ -lm -fopenmp

# object rule
$(BUILD)/%.o: $(SRC_DIR)/%.c | $(BUILD)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# build dir
$(BUILD):
	mkdir -p $(BUILD)

# convenience: run with your config file
run: $(TARGET)
	ulimit -n 4096 && ./$(TARGET) default_config.txt
clean:
	rm -rf $(BUILD) $(TARGET)

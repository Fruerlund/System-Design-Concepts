SRC_DIR = src
BUILD_DIR = build

HEADERS = $(wildcard $(SRC_DIR)/include/*.h)
C_SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(C_SOURCES))

# COMPILER
CC = /usr/bin/gcc

# COMPILER FLAGS
CFLAGS = -g -I src/include -pthread

# Final executable
TARGET = bin/main

all: $(TARGET)

$(TARGET): $(OBJ)
	${CC} ${CFLAGS} -o $@ $^


$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

loadbalancer: $(SRC_DIR)/loadbalancer.c
	$(CC) $(CFLAGS) $< -o $(BUILD_DIR)/$@
clean:
	echo "Cleaning"
	rm -rf bin/* build/

run:
	bin/main
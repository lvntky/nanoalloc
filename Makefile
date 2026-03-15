CC      = gcc
CFLAGS  = -O2 -g -Wall -Wextra -fPIC -I./include
LDFLAGS = -shared -ldl -lpthread

SRC     = src/hook.c
TARGET  = na_alloc.so

.PHONY: all clean test smoke

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^
	@echo "Built $(TARGET)"

# Quick smoke test: run ls under our allocator with stats enabled
smoke: $(TARGET)
	@echo "\n── smoke test: ls ──"
	NA_STATS=1 LD_PRELOAD=./$(TARGET) ls /usr/bin | wc -l

# More realistic: python importing a module
python-test: $(TARGET)
	@echo "\n── smoke test: python3 ──"
	NA_STATS=1 LD_PRELOAD=./$(TARGET) python3 -c \
		"import os; print('python ok, pid=', os.getpid())"

# Show every malloc call's size (requires NA_VERBOSE=1, add it later)
verbose-test: $(TARGET)
	NA_STATS=1 LD_PRELOAD=./$(TARGET) $(CMD)

clean:
	rm -f $(TARGET)

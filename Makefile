CC := gcc
CFLAGS := -g -Wall -Wextra -pedantic -O2

SRCS := cc.c cache.c

OUTPUT := CC-Simulate
DEBUG := DCC-Simulate

all: $(OUTPUT) $(DEBUG)

$(OUTPUT): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^

$(DEBUG): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^ -DDEBUG

clean:
	rm -f $(OUTPUT) $(DEBUG)

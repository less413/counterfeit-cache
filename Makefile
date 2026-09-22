CC := gcc
CFLAGS := -Wall -Wextra -pedantic -O2

SRCS := cc.c cache.c

OUTPUT := CC-Simulate
DEBUG := DCC-Simulate

all: $(OUTPUT) $(DEBUG)

$(OUTPUT): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^

$(DEBUG): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^ -DDEBUG

old: ccc.c 
	$(CC) $(CFLAGS) -o old $^

clean:
	rm -f $(OUTPUT) $(DEBUG)

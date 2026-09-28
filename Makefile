CC := gcc
CFLAGS := -g -Wall -Wextra -pedantic -O2

SRCS := cc.c cache.c
HDRS := cc.h cache.h

OUTPUT := CC-Simulate
DEBUG := DCC-Simulate

all: $(OUTPUT) $(DEBUG)

$(OUTPUT): $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) -o $@ $(SRCS)

$(DEBUG): $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) -o $@ $(SRCS) -DDEBUG

clean:
	rm -f $(OUTPUT) $(DEBUG)

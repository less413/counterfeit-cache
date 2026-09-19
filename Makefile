CC := gcc
CFLAGS := -Wall -Wextra -pedantic -O2

OUTPUT_CACHE_SIM := CC-Simulate

all: $(OUTPUT_CACHE_SIM)

$(OUTPUT_CACHE_SIM): cc.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(OUTPUT_CACHE_SIM)

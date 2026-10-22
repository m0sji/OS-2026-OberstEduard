CC     = gcc
CFLAGS = -Wall -Wextra

PROGS = run zombie redirect pipeline pipeline3

all: $(PROGS)

%: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(PROGS) out.txt

.PHONY: all clean

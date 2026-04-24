CFLAGS = -std=c99 -Wall -Wextra -Werror
OBJS = main.o arithmetic.o utils.o demo.o tests.o unity.o

prog: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJS) prog

.PHONY: clean

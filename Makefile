CFLAGS = -Wall -Wextra -Werror
OBJS = main.o arithmetic.o utils.o demo.o

prog: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJS) prog

.PHONY: clean

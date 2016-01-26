CC=g++
CFLAGS= -Wall

SRC=$(wildcard *.c)
OBJS=$(SRC:.c=.o)

http: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

$(OBJS) : %.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o

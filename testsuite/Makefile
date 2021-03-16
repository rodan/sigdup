PROG = rz
OBJS = zcrc.o zmodem.o

CC = gcc
CFLAGS= -Wall -DHOST -ggdb
LDFLAGS =

.c.o:
	$(CC) -c $(CFLAGS) $*.c

$(PROG): tags $(OBJS)
	$(CC) $(LDFLAGS) -lutil -o $(PROG) $(OBJS) $(LIBS)

tags:
	@exuberant-ctags -R *

scan-build: clean
	@scan-build --use-cc=$(CC) make PARSER=scan-build

cppcheck: clean
	@cppcheck --force --enable=warning --enable=performance --enable=information --enable=missingInclude -I ./ ./

clean :
	rm -f *.o $(PROG) tags

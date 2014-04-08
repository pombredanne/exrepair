# Makefile

SRCS	= exrepair.c exdespair.c cpm.c cpm8.c simple_trie.c cont.c bits.c timer.c
TARGET	= exrepair exdespair

OBJS	= $(SRCS:%.c=%.o)
CC	= gcc
CFLAGS	= -O2 -DNDEBUG -Wall
LIB	= -lm

all: 
	make clean
	make $(TARGET)

exrepair: main.c exrepair.o cont.o bits.o
	$(CC) $(CFLAGS) -DCREPAIR -o $@ $^ $(LIB)

exdespair: main.c exdespair.o cont.o bits.o
	$(CC) $(CFLAGS) -DCDESPAIR -o $@ $^ $(LIB)

cpm: main.c cpm.o bits.o timer.o
	$(CC) $(CFLAGS) -DCPM -o $@ $^ $(LIB)

cpm8: main.c cpm8.o timer.o
	$(CC) $(CFLAGS) -DCPM8 -o $@ $^ $(LIB)

clean:
	-rm -f $(TARGET) $(OBJS)  *~

.c.o:
	$(CC) $(CFLAGS) -c $<

exrepair.o: exrepair.h cont.h bits.h basics.h
exdespair.o: basics.h cont.h bits.h basics.h
cpm.o: basics.h bits.h cpm.h basics.h
cpm8.o: basics.h cpm8.h basics.h
simple_trie.o: basics.h
getID.o: basics.h
bits.o: bits.h basics.h
timer.o: timer.h



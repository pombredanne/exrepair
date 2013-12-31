# Makefile

SRCS	= exrepair.c exdespair.c cpm.c cpm8.c bits.c timer.c
TARGET	= exrepair exdespair cpm cpm8

OBJS	= $(SRCS:%.c=%.o)
CC	= g++
CFLAGS	= -O3 -fomit-frame-pointer -DNDEBUG -Wall
LIB	= -lm

all: 
	make clean
	make $(TARGET)

exrepair: main.cpp exrepair.o bits.o
	$(CC) $(CFLAGS) -DCREPAIR -o $@ $^ $(LIB)

exdespair: main.cpp exdespair.o bits.o
	$(CC) $(CFLAGS) -DCDESPAIR -o $@ $^ $(LIB)

cpm: main.cpp cpm.o bits.o timer.o
	$(CC) $(CFLAGS) -DCPM -o $@ $^ $(LIB)

cpm8: main.cpp cpm8.o timer.o
	$(CC) $(CFLAGS) -DCPM8 -o $@ $^ $(LIB)

clean:
	-rm -f $(TARGET) $(OBJS)  *~

.c.o:
	$(CC) $(CFLAGS) -c $<

exrepair.o: exrepair.h bits.h
exdespair.o: exdespair.h basics.h bits.h
cpm.o: basics.h bits.h cpm.h
cpm8.o: basics.h cpm8.h
bits.o: bits.h basics.h
timer.o: timer.h



CFLAGS?=-O0 -g -Wall -W $(shell pkg-config --cflags librtlsdr libhackrf)
LDLIBS+=$(shell pkg-config --libs librtlsdr libhackrf) -lpthread -lm
CC?=gcc
PROGNAME=main

all: main

# %.o: %.c
# 	$(CC) $(CFLAGS) -c $<

# main: main.o anet.o
# 	$(CC) -g -o main main.o anet.o $(LDFLAGS) $(LDLIBS)

# clean:
# 	rm -f *.o main

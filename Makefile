CC=gcc
CFLAGS=-Wall -g
RM=rm -f
GOOCAA=goocaa

all:
	gcc ${CFLAGS} `pkg-config neon libxml-2.0 glib-2.0 --libs --cflags` -o ${GOOCAA} main.c google.c

clean:
	${RM} ${GOOCAA}
	${RM} *.core	

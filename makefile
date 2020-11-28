#
CFLAGS = -g
#CFLAGS =
CC = cc
LIBS = -lm 

.SUFFIXES:	.o .c .h


.c.o:
	$(CC) $(CFLAGS) -c $*.c 


default:schedule

schedule:	schedule.o greg.o
	$(CC) $(CFLAGS) -o schedule schedule.o greg.o $(LIBS)

schedule.o:	greg.o
	$(CC) $(CFLAGS) -c schedule.c $(LIBS)

greg.o:	greg.c 
	$(CC) $(CFLAGS) -c greg.c $(LIBS)

iqprt:	iqprt.o
	$(CC) $(CFLAGS) -o iqprt iqprt.o $(LIBS)

vnet:	vnet.o rmb.o short.o
	$(CC) $(CFLAGS) -o vnet vnet.o rmb.o short.o $(LIBS)

s2  :	s2.o 
	$(CC) $(CFLAGS) -o s2 s2.o $(LIBS)

p2q :	p2q.o
	$(CC) $(CFLAGS) -o p2q p2q.o $(LIBS)

newv:	newv.o
	$(CC) $(CFLAGS) -o newv newv.o $(LIBS)

test:	test.o
	$(CC) $(CFLAGS) -o test test.o $(LIBS)

jest:	jest.o
	$(CC) $(CFLAGS) -o jest jest.o $(LIBS)

CC      = gcc
CFLAGS  = -g -Wall -I/usr/include/tirpc
LDLIBS  = -ltirpc -lnsl -lpthread

all: squid_server squid_client

squid_server: squid_server.o squid_svc.o squid_xdr.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

squid_client: squid_client.o squid_clnt.o squid_xdr.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

squid_server.o: squid_server.c squid.h
	$(CC) $(CFLAGS) -c -o $@ squid_server.c

squid_client.o: squid_client.c squid.h
	$(CC) $(CFLAGS) -c -o $@ squid_client.c

squid_svc.o: squid_svc.c squid.h
	$(CC) $(CFLAGS) -c -o $@ squid_svc.c

squid_clnt.o: squid_clnt.c squid.h
	$(CC) $(CFLAGS) -c -o $@ squid_clnt.c

squid_xdr.o: squid_xdr.c squid.h
	$(CC) $(CFLAGS) -c -o $@ squid_xdr.c

clean:
	rm -f *.o squid_server squid_client

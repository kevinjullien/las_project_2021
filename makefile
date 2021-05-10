CFLAGS=-D_DEFAULT_SOURCE -D_XOPEN_SOURCE -D_BSD_SOURCE -std=c11 -pedantic -Wvla -Wall -Werror
ALL=./server/maint ./server/stat ./server/server ./client/client

all: $(ALL)
	ipcs

./server/maint: ./server/maint.c utils_v10.h utils_v10.o ./server/ipc_conf.h
	cc $(CFLAGS) -o ./server/maint ./server/maint.c utils_v10.o

./server/server: ./server/server.c utils_v10.h utils_v10.o ./server/ipc_conf.h messages.h
	cc $(CFLAGS) -o ./server/server ./server/server.c utils_v10.o

./server/stat: ./server/stat.c utils_v10.h utils_v10.o ./server/ipc_conf.h
	cc $(CFLAGS) -o ./server/stat ./server/stat.c utils_v10.o

./client/client: ./client/client.c utils_v10.h utils_v10.o
	cc $(CFLAGS) -o ./client/client ./client/client.c utils_v10.o 

utils_v10.o: utils_v10.h utils_v10.c 
	cc $(CFLAGS) -c utils_v10.c


clean:
	rm -f *.o ./server/*.o $(ALL)
	ipcrm -a

create_server:
	./server/maint 1 ; ./server/server 9090

create_client:
	./client/client 127.0.01 9090 5

close_serveur:
	./server/maint 2

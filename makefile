CFLAGS=-D_DEFAULT_SOURCE -D_XOPEN_SOURCE -D_BSD_SOURCE -std=c11 -pedantic -Wvla -Wall -Werror
<<<<<<< HEAD
ALL=./server/maint ./server/server ./server/stat ./client/client
=======
ALL=./server/maint ./server/stat #./server/server  ./client/client
>>>>>>> 19aad0f96b74ed351b2907c9802b322479e7ef85

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
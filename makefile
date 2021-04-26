CFLAGS=-D_DEFAULT_SOURCE -D_XOPEN_SOURCE -D_BSD_SOURCE -std=c11 -pedantic -Wvla -Wall -Werror
ALL=maint server stat
SERVER=./server/maint ./server/server ./server/stat

all: $(ALL)
	ipcs

maint: ./server/maint.c utils_v10.h utils_v10.o
	cc $(CFLAGS) -o ./server/maint ./server/maint.c utils_v10.o

server: ./server/server.c utils_v10.h utils_v10.o
	cc $(CFLAGS) -o ./server/server ./server/server.c utils_v10.o

stat: ./server/stat.c utils_v10.h utils_v10.o
	cc $(CFLAGS) -o ./server/stat ./server/stat.c utils_v10.o 

utils_v10.o: utils_v10.h utils_v10.c 
	cc $(CFLAGS) -c utils_v10.c

clean:
	rm -f ./server/*.o $(SERVER)
	ipcrm -a
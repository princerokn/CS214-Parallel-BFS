CC = g++
CPFLAGS = -DHOMEGROWN
LPFLAGS = -pthread
CFLAGS = -mcx16 -O3 -std=c++17 -march=native -Wall $(CPFLAGS) 

all:	bfs

bfs:	bfs.cpp bfs.h
	$(CC) $(CFLAGS) bfs.cpp -o bfs $(LPFLAGS)

clean:
	rm -f bfs

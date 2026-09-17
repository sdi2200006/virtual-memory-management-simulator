all: src/main.o src/MM.o src/IPC.o src/PM1.o src/PM2.o main pm1 pm2

src/main.o:
	gcc -Wall -c -o src/main.o src/main.c

src/MM.o:
	gcc -Wall -c -o src/MM.o src/MM.c

src/IPC.o: 
	gcc -Wall -c -o src/IPC.o src/IPC.c

src/PM1.o:
	gcc -Wall -c -o src/PM1.o src/PM1.c

src/PM2.o:
	gcc -Wall -c -o src/PM2.o src/PM2.c

main: 
	gcc -Wall -o main src/main.o src/MM.o src/IPC.o -pthread -lrt

pm1: 
	gcc -Wall -o pm1 src/PM1.o src/IPC.o -pthread -lrt

pm2: 
	gcc -Wall -o pm2 src/PM2.o src/IPC.o -pthread -lrt

clean:
	rm -f src/*.o main pm1 pm2

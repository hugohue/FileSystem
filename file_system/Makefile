CC=gcc
CFLAGS=  
TARGET=recover

all: main.o recover.o disk_info.o
	$(CC) -o $(TARGET) $^
main.o: main.c disk_info.h
	$(CC) -c $< $(CFLAGS)
recover.o: recover.c disk_info.h
	$(CC) -c $< $(CFLAGS)
disk_info.o: disk_info.c disk_info.h
	$(CC) -c $< $(CFLAGS)
clean:
	rm -rf $(TARGET)
	rm -rf *.o

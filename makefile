GCC := arm-linux-gnueabihf-gcc
OUTPUT := thread_dht
SOURCES := $(wildcard *.c)
CCFLAGS := -pthread -lwiringPi -Wall

all: 
	$(GCC) -o $(OUTPUT) $(CCFLAGS) $(SOURCES)

clean:
	rm $(OUTPUT)
	
.PHONY: all
 

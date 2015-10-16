
all: mp751

mp751: mp751.o hid-libusb.o
	gcc mp751.o hid-libusb.o -o mp751 -lusb-1.0 -lrt -lpthread

mp751.o: mp751.c
	gcc -c mp751.c
	
hid-libusb.o: hid-libusb.c 
	gcc -I/usr/include/libusb-1.0 -I. -c hid-libusb.c

clean:
	rm -f hid-libusb.o mp751.o mp751

.PHONY: clean
obj-m := message_slot.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	gcc -O3 -Wall -std=c11 -o reader message_reader.c
	gcc -O3 -Wall -std=c11 -o sender message_sender.c

 
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm reader
	rm sender

programs: message-sender message-reader
message-sender: message_sender.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^
message-reader: message_reader.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^
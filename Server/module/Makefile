# Kernel module variables
obj-m := lacpm.o
ccflags-y += -g -I$(src)
KVERSION = $(shell uname -r)

all:
	make -C /lib/modules/$(KVERSION)/build M=$(CURDIR) modules

install:
	make -C /lib/modules/$(KVERSION)/build M=$(CURDIR) modules_install
	depmod -a 
	modprobe lacpm

uninstall:
	rmmod lacpm

install-headers:
	cp -rf ../module/lacpm /usr/include/

clean:
	make -C /lib/modules/$(KVERSION)/build M=$(CURDIR) clean
	rm -rf lacpm.o .lacpm.o.cmd

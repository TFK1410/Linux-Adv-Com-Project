# Kernel module variables
obj-m := lacpm.o
ccflags-y += -g -I$(src)

HOST_KERNEL_BUILD_DIR = /lib/modules/$(shell uname -r)/build
TARGET_KERNEL_BUILD_DIR = $(BUILDROOT_PATH)/output/build/linux-4.3

host:
	make -C $(HOST_KERNEL_BUILD_DIR) M=$(CURDIR) modules

target:
	make -C $(TARGET_KERNEL_BUILD_DIR) M=$(CURDIR) modules

clean:
	make -C $(HOST_KERNEL_BUILD_DIR) M=$(CURDIR) clean
	rm -rf lacpm.o .lacpm.o.cmd

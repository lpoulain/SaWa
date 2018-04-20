TARGET_MODULE:=sawa-device

# If we are running by kernel building system
ifneq ($(KERNELRELEASE),)
	$(TARGET_MODULE)-objs := main.o sawa.o tcp.o
	obj-m := $(TARGET_MODULE).o
# If we running without kernel build system
else
	BUILDSYSTEM_DIR:=/lib/modules/$(shell uname -r)/build
	PWD:=$(shell pwd)

all:
# run kernel build system to make module
	$(MAKE) -C $(BUILDSYSTEM_DIR) M=$(PWD) modules

clean:
# run kernel build system to cleanup in current directory
	$(MAKE) -C $(BUILDSYSTEM_DIR) M=$(PWD) clean

ins:
	dmesg -C
	insmod ./$(TARGET_MODULE).ko

setup:
	( echo n; echo; echo; echo; echo; echo w ) | fdisk /dev/sawa0
	mkfs /dev/sawa0

mount:
	mount /dev/sawa0 /mnt/test

umount:
	umount /mnt/test

end:
	sudo rmmod ./$(TARGET_MODULE).ko

endif

TARGET_MODULE := go
#main-objs := ramdisk.o
obj-m := $(TARGET_MODULE).o
go-objs := main.o ramdisk.o

BUILDSYSTEM_DIR:=/lib/modules/$(shell uname -r)/build
all : 
	$(MAKE) -C  $(BUILDSYSTEM_DIR) M=$(PWD) modules
clean:
	$(MAKE) -C  $(BUILDSYSTEM_DIR) M=$(PWD) clean
load:
	dmesg -C
	insmod ./$(TARGET_MODULE).ko memory_size=1024 back_major=8 back_minor=32
	./format.sh
	mkfs.ext4 /dev/bestioa1
	mount /dev/bestioa1 usb

unload:
	umount usb
	rmmod ./$(TARGET_MODULE).ko
	dmesg

test:
	$(MAKE) load
	$(MAKE) unload


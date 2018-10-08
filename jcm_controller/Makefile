obj-m := vrd_2018_04_18.o
#test-objs:= vrd_2018_04_18.o
BUILDSYSTEM_DIR:=/lib/modules/$(shell uname -r)/build
all : 
	$(MAKE) -C $(BUILDSYSTEM_DIR) M=$(PWD) modules
clean:
	$(MAKE) -C $(BUILDSYSTEM_DIR) M=$(PWD) clean


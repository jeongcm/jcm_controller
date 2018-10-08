TARGET=jcm_controller
CC=gcc
OBJS = jcm_controller.o json_file.o dmbd_hb.o


$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS) -ljson-c -lpthread

controller.o: json_file.h jcm_controller.h jcm_controller.c
json_file.o: json_file.h jcm_controller.h json_file.c
#dmbd_hb.o: json_file.h dmbd_hb.h dmbd_ioctl.h json_file.c dmbd_hb.c -ljson-c -lpthread

clean:
	rm -f *.o
	rm -f $(TARGET)
pclean:
	pkill -9 nbd-server
	pkill -9 nbd-client
m_make:
	modprobe nbd
	iptables -A INPUT -p tcp --dport 1234 -j ACCEPT
	iptables -A OUTPUT -p tcp --dport 1234 -j ACCEPT
	iptables -A INPUT -p tcp --dport 7431 -j ACCEPT
	iptables -A OUTPUT -p tcp --dport 7431 -j ACCEPT

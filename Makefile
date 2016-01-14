CFLAGS  += -g

all:    tcp_tproxy udp_tproxy

tcp_tproxy:     tcp_tproxy.o
	$(CC) -o $@ tcp_tproxy.o

udp_tproxy:     udp_tproxy.o
	$(CC) -o $@ udp_tproxy.o

clean:
	-@rm *.o

distclean:      clean
	-@rm tcp_tproxy udp_tproxy

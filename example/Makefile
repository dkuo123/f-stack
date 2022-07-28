TOPDIR=..

ifeq ($(FF_PATH),)
	FF_PATH=${TOPDIR}
endif

ifeq ($(FF_DPDK),)
	FF_DPDK=${TOPDIR}/dpdk/x86_64-native-linuxapp-gcc
endif

LIBS+= -L${FF_PATH}/lib -Wl,--whole-archive,-lfstack,--no-whole-archive
LIBS+= -L${FF_DPDK}/lib -Wl,--whole-archive,-ldpdk,--no-whole-archive
LIBS+= -Wl,--no-whole-archive -lrt -lm -ldl -lcrypto -pthread -lnuma

TARGET="helloworld"
all:
	cc -O -gdwarf-2  -I../lib -o ${TARGET} main.c ${LIBS}
	cc -O -gdwarf-2  -I../lib -o ${TARGET}_epoll main_epoll.c ${LIBS}
	# cc -O -gdwarf-2  -I../lib -o ff_server ff_server.c ${LIBS}
	# cc -O -gdwarf-2  -I../lib -o raw_server raw_server.c ${LIBS}
	g++ -std=c++17 -O2 -I../lib -o ff_server ff_server.c ${LIBS}
	g++ -std=c++17 -O2 -I../lib -o raw_server raw_server.c ${LIBS}
	g++ -std=c++17 -O2 -I../lib -o ff_epoll_server ff_epoll_server.cpp ${LIBS}

.PHONY: clean
clean:
	rm -f *.o ${TARGET} ${TARGET}_epoll

TOPDIR=../..

ifeq ($(FF_PATH),)
	FF_PATH=${TOPDIR}
endif

ifeq ($(FF_DPDK),)
	FF_DPDK=${TOPDIR}/dpdk/x86_64-native-linuxapp-gcc
endif

LIBS+= -L${FF_PATH}/lib -Wl,--whole-archive,-lfstack,--no-whole-archive
LIBS+= -L${FF_DPDK}/lib -Wl,--whole-archive,-ldpdk,--no-whole-archive
LIBS+= -Wl,--no-whole-archive -lrt -lm -ldl -lcrypto -pthread -lnuma

all:
	g++ -std=c++17 -O2 -I${FF_PATH}/lib -o socket_epoll_server epoll_server.cpp ${LIBS}
	# g++ -std=c++17 -O2 -I${FF_PATH}/lib -DF_STACK -o ff_epoll_server epoll_server.cpp ${LIBS}
	g++ -std=c++17 -O2 -o receiving_only_client receiving_only_client.cpp
.PHONY: clean
clean:
	rm -f *.o ${TARGET} ${TARGET}_epoll

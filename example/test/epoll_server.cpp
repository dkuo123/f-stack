#include <stdio.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <assert.h>

#include <time.h>
#include <chrono>
#include <thread>

#ifdef F_STACK  // using F-stack
#include "ff_config.h"
#include "ff_api.h"
#include "ff_epoll.h"

// #define MAXEPOLLSIZE 0

#define PROG_init ff_init(argc, argv)
#define PROG_socket ff_socket
#define PROG_ioctl ff_ioctl
#define PROG_bind ff_bind
#define PROG_listen ff_listen
#define PROG_epoll_create ff_epoll_create
#define PROG_epoll_ctl ff_epoll_ctl
#define PROG_epoll_wait ff_epoll_wait
#define PROG_accept ff_accept
#define PROG_close ff_close
#define PROG_send ff_send
#define PROG_write ff_write
#define PROG_read ff_read
#define PROG_run(first, second) ff_run(first, second)

#else   // using regular socket
#include <unistd.h>
#include <sys/epoll.h>

#define linux_sockaddr sockaddr
// #define MAXEPOLLSIZE 1

#define PROG_init ;
#define PROG_socket socket
#define PROG_ioctl ioctl
#define PROG_bind bind
#define PROG_listen listen
#define PROG_epoll_create epoll_create1
#define PROG_epoll_ctl epoll_ctl
#define PROG_epoll_wait epoll_wait
#define PROG_accept accept
#define PROG_close close
#define PROG_send send
#define PROG_write write
#define PROG_read read
#define PROG_run(first, second) while (true) { first(second); }
#endif

#ifdef USE_UDP
#define SOCKET_TYPE SOCK_DGRAM
#else
#define SOCKET_TYPE SOCK_STREAM
#endif

#include "comm_def.h"

// #define MAX_EVENTS 512

struct epoll_event ev;
struct epoll_event events[MAX_EVENTS];

int epfd;
int sockfd;

using namespace std::chrono_literals;

int loop(void *arg)
{
    /* Wait for events to happen */
    TestData out;
    memset(&out, 0, sizeof(out));
    int nevents = PROG_epoll_wait(epfd,  events, MAX_EVENTS, 0);
    int i;

    for (i = 0; i < nevents; ++i) {
        /* Handle new connect */
        if (events[i].data.fd == sockfd) {
            while (1) {
                int nclientfd = PROG_accept(sockfd, NULL, NULL);
                if (nclientfd < 0) {
                    break;
                }

                // /* Add to event list */
                ev.data.fd = nclientfd;
                ev.events  = EPOLLIN;
                if (PROG_epoll_ctl(epfd, EPOLL_CTL_ADD, nclientfd, &ev) != 0) {
                    printf("PROG_epoll_ctl failed:%d, %s\n", errno,
                        strerror(errno));
                    break;
                }
            }
        } else {
            if (events[i].events & EPOLLERR ) {
                /* Simply close socket */
                PROG_epoll_ctl(epfd, EPOLL_CTL_DEL,  events[i].data.fd, NULL);
                PROG_close(events[i].data.fd);
            } else if (events[i].events & EPOLLIN) {
                char buf[256];
                size_t readlen = PROG_read( events[i].data.fd, buf, sizeof(buf));
                if(readlen > 0) {
                    // printf("client sent:%s\n", buf);

                    time_t start = time(NULL);
                    for (int k = 0; k < BATCH; ++k, ++out.seq) {
                        // printf("sent %d\n", out.seq);
                        // auto ret = PROG_write( events[i].data.fd, &out, sizeof(out));
                        auto ret = PROG_send( events[i].data.fd, &out, sizeof(out), MSG_DONTWAIT);
                        if ( ret != sizeof(out)) {
                            printf("ERROR only sent out %d bytes, %s\n", ret, strerror(errno));
                        }
                        // std::this_thread::sleep_for(10ms);
                    }
                    time_t end = time(NULL);
                    // printf("sent %d packages, duration in seconds=%ld\n",
                    //         BATCH, end - start);
                } else {
                    PROG_epoll_ctl(epfd, EPOLL_CTL_DEL,  events[i].data.fd, NULL);
                    PROG_close( events[i].data.fd);
                }

            } else {
                printf("unknown event: %8.8X\n", events[i].events);
            }
        }
    }
    return 0;
}

int main(int argc, char * argv[])
{
    PROG_init; // (argc, argv);

    sockfd = PROG_socket(AF_INET, SOCKET_TYPE, 0);
    printf("sockfd:%d\n", sockfd);
    if (sockfd < 0) {
        perror("PROG_socket failed\n");
        exit(1);
    }

    int on = 1;
    PROG_ioctl(sockfd, FIONBIO, &on);

    struct sockaddr_in my_addr;
    bzero(&my_addr, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(SERVER_PORT);
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int ret = PROG_bind(sockfd, (struct linux_sockaddr *)&my_addr, sizeof(my_addr));
    if (ret < 0) {
        perror("PROG_bind failed\n");
        exit(1);
    }

    ret = PROG_listen(sockfd, MAX_EVENTS);
    if (ret < 0) {
        perror("PROG_listen failed\n");
        exit(1);
    }

    assert((epfd = PROG_epoll_create(0)) > 0);
    ev.data.fd = sockfd;
    ev.events = EPOLLIN;
    PROG_epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev);
    PROG_run(loop, NULL);
    return 0;
}

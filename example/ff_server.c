#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <arpa/inet.h>

#include "ff_config.h"
#include "ff_api.h"

#include "comm_def.h"

struct kevent _kset = {0};
struct kevent _kevs[MAX_EVENTS] = {{0}};
int _kfd = 0;
int _sfd = 0;

int loop(void *arg)
{
    char buf[MAX_DATA_LEN] = {0};
    int nevents = ff_kevent(_kfd, NULL, 0, _kevs, MAX_EVENTS, NULL);
    static int k = 0;

    for (int k = 0; k < nevents; ++k)
    {
        struct kevent event = _kevs[k];
        int curfd = (int)event.ident;
        if (event.flags & EV_EOF)
        {
            printf("INFO! client exit. fd=%d\n", curfd);
            k = 0;
            ff_close(curfd);
        }
        else if (curfd == _sfd)
        {
            int available = (int)event.data;
            do
            {
                int newfd = ff_accept(curfd, NULL, NULL);
                if (newfd < 0)
                {
                    printf("ERROR! ff_accept failed! error=%d %s\n", errno, strerror(errno));
                    k = 0;
                    break;
                }

                EV_SET(&_kset, newfd, EVFILT_READ, EV_ADD, 0, 0, NULL);

                if (ff_kevent(_kfd, &_kset, 1, NULL, 0, NULL) < 0)
                {
                    printf("ERROR! ff_kevent failed! error=%d %s\n", errno, strerror(errno));
                    k = 0;
                    return -1;
                }

                printf("INFO! accept. newfd=%d ava=%d event=%d\n", newfd, available, nevents);
                available--;
            } while (available);
        }
        else if (event.filter == EVFILT_READ)
        {
            ++k;
            size_t readlen = ff_recv(curfd, buf, MAX_DATA_LEN, 0);
            printf("%d time got data, len=%d\n", k, readlen);
            ff_send(curfd, buf, readlen, 0); //echo
        }
        else if (event.filter == EVFILT_WRITE)
        {
            char* msg = "hi, welcome connect!";
            ff_write(curfd, msg, strlen(msg));
        }
        else
        {
            printf("WARN! unknown event=%8.8X\n", event.flags);
        }
    }

    return 0;
}

int main(int argc, char * argv[])
{
    ff_init(argc, argv);

    _sfd = ff_socket(AF_INET, SOCK_STREAM, 0);
    if (_sfd < 0)
    {
        printf("ERROR! ff_socket failed! sockfd=%d errno=%d %s\n", _sfd, errno, strerror(errno));
        exit(1);
    }
    printf("INFO! ff_socket init ok! fd=%d\n", _sfd);

    struct sockaddr_in my_addr = {0};
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP_eth1, &my_addr.sin_addr) <= 0)
    {
        printf("ERROR! inet_pton failed! sockfd=%d errno=%d %s\n", _sfd, errno, strerror(errno));
        exit(1);
    }

    int ret = ff_bind(_sfd, (struct linux_sockaddr *)&my_addr, sizeof(my_addr));
    if (ret < 0)
    {
        printf("ERROR! ff_bind failed! sockfd=%d errno=%d %s\n", _sfd, errno, strerror(errno));
        exit(1);
    }

    ret = ff_listen(_sfd, MAX_EVENTS);
    if (ret < 0)
    {
        printf("ERROR! ff_listen failed! sockfd=%d errno=%d %s\n", _sfd, errno, strerror(errno));
        exit(1);
    }

    EV_SET(&_kset, _sfd, EVFILT_READ, EV_ADD, 0, MAX_EVENTS, NULL);

    _kfd = ff_kqueue();
    if (_kfd <= 0)
    {
        printf("ERROR! ff_kqueue failed! sockfd=%d errno=%d %s\n", _sfd, errno, strerror(errno));
        ff_close(_sfd);
        exit(1);
    }
    ff_kevent(_kfd, &_kset, 1, NULL, 0, NULL);
    printf("INFO! prepare loop! fd=%d kq=%d\n", _sfd, _kfd);
    ff_run(loop, NULL);

    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include <arpa/inet.h>

#include "comm_def.h"

int _sfd = 0;

int main(int argc, char * argv[])
{
    _sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (_sfd < 0)
    {
        printf("ERROR! socket failed! sockfd=%d errno=%d %s\n", _sfd, errno, strerror(errno));
        exit(1);
    }
    printf("INFO! socket init ok! fd=%d\n", _sfd);

    struct sockaddr_in my_addr = {0};
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP_eth0, &my_addr.sin_addr) <= 0)
    {
        printf("ERROR! inet_pton failed! sockfd=%d errno=%d %s\n", _sfd, errno, strerror(errno));
        exit(1);
    }

    int ret = bind(_sfd, (struct sockaddr *)&my_addr, sizeof(my_addr));
    if (ret < 0)
    {
        printf("ERROR! bind failed! sockfd=%d errno=%d %s\n", _sfd, errno, strerror(errno));
        exit(1);
    }

    ret = listen(_sfd, 10);
    if (ret < 0)
    {
        printf("ERROR! listen failed! sockfd=%d errno=%d %s\n", _sfd, errno, strerror(errno));
        exit(1);
    }

    signal(SIGPIPE, SIG_IGN);

    int cfd = 0;
    char buf[MAX_DATA_LEN] = {0};
    struct sockaddr_in remote_addr = {0};
    for (; ;)
    {
        socklen_t addrlen = sizeof(struct sockaddr);
        cfd = accept(_sfd, (struct sockaddr*)&remote_addr, &addrlen);
        printf("INFO! accept. cfd=%d\n", cfd);
        int k = 0;
        for (; ;)
        {
            ret = read(cfd, buf, MAX_DATA_LEN);
            if (ret <= 0 )
            {
                printf("INFO! client exit. cfd=%d\n", cfd);
                break;
            }
            else {
                ++k;
                printf("%d time got data, len=%d\n", k, ret);
            }
            write(cfd, buf, ret);
        }
        close(cfd);
    }

    return 0;
}

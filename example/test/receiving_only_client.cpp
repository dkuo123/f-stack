#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <string_view>
//  g++ -O2 -std=c++17 -o receiving_only_client receiving_only_client.cpp
#include <sys/socket.h>
#include <arpa/inet.h>
#include "comm_def.h"

std::string_view hello_msg = "Login from client";

int main(int argc, char *argv[])
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        printf("ERROR! socket failed! sockfd=%d errno=%d %s\n", sockfd, errno, strerror(errno));
        exit(1);
    }
    printf("INFO! socket init ok! fd=%d\n", sockfd);

    struct sockaddr_in serv_addr = {0};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);
    const char * SERVER_IP;
    if (argc > 1) // raw socket server
        SERVER_IP = SERVER_IP_eth0;
    else
        SERVER_IP = SERVER_IP_eth1;
    printf("start server on ip=%s, port=%d\n", SERVER_IP, SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0)
    {
        printf("ERROR! inet_pton failed! fd=%d\n", sockfd);
        exit(1);
    }

    printf("INFO! prepare connect server...\n");
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("ERROR! connect failed! fd=%d\n", sockfd);
        exit(1);
    }
    printf("INFO! connect ok! fd=%d\n", sockfd);

    auto ret = write(sockfd, hello_msg.data(), hello_msg.size());
    if (ret <= 0)
    {
        printf("ERROR! write failed! sockfd=%d errno=%d %s\n", sockfd, errno, strerror(errno));
        exit(1);
    }

    TestData *received;
    size_t expected = sizeof(TestData);
    char buffer[64*sizeof(TestData)];
    int head = 0, next = 0;

    time_t start = time(NULL);
    for (int k = 0; k < TEST_TIMES; ++k) {
        printf("testing loop# %d\n", k);
        auto ret = write(sockfd, hello_msg.data(), hello_msg.size());
        if (ret <= 0) {
            printf("ERROR! write failed! sockfd=%d errno=%d %s\n", sockfd, errno, strerror(errno));
            exit(1);
        }
        for ( int i = 0; i < BATCH; ) {
            ret = read(sockfd, buffer+next, expected);
            // printf("reading package len = %d\n", ret);
            if (ret > expected) {
                printf("ERROR! expect size =%lu got larger=%lu\n", expected, ret);
                break;
            } else {
                // printf("%d, reading %s package len = %d\n", i,
                //     (ret < expected? "partial":"full"), ret);
                next += ret;
            }
            head = 0;
            while (next >= expected) {
                received = reinterpret_cast<TestData *>(buffer+head);
                if (i != received->seq) {
                    printf("ERROR! expect seq=%d got=%d\n", i, received->seq);
                    return 1;
                } else {
                    // printf("got %d\n", i);
                    ++i;
                    head += expected;
                    next -= expected;
                }
            }
            if (next != 0) {
                memmove(buffer, buffer+head, next);
            }
        }
    }

    time_t end = time(NULL);
    printf("send %d times, each time BATCH=%d, duration in seconds=%ld\n",
            TEST_TIMES, BATCH, end - start);

    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

// gcc -DUSE_RAW -o raw_client echo_client.c
// gcc -o ff_client echo_client.c
#include <sys/socket.h>
#include <arpa/inet.h>

#define SERVER_PORT     54321
#define MAX_DATA_LEN    1024  // 4096
#define MAX_EVENTS      8
#define TEST_TIMES      10000

#ifdef USE_RAW
const char * SERVER_IP  = "10.50.12.133";
#else
const char * SERVER_IP  = "10.50.12.75";
#endif

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

    char buf[MAX_DATA_LEN] = {0};
    char data[MAX_DATA_LEN + 1] = {0};
    for (int k = 0; k < MAX_DATA_LEN; ++k)
    {
        data[k] = rand() % 10 + '0';
    }
    int sendlen = strlen(data);
    int ret = 0;
    time_t start = time(NULL);
    long int sendsum = 0;
    long int recvsum = 0;
    for (int k = 0; k < TEST_TIMES; ++k)
    {
        printf("testing k=%d\n", k);
        ret = write(sockfd, data, sendlen);
        if (ret <= 0)
        {
            printf("ERROR! write failed! sockfd=%d errno=%d %s\n", sockfd, errno, strerror(errno));
            break;
        }
        sendsum += ret;

        ret = read(sockfd, buf, ret);
        if (ret <= 0)
        {
            printf("ERROR! read failed! sockfd=%d errno=%d %s\n", sockfd, errno, strerror(errno));
            break;
        }
        recvsum += ret;
    }
    time_t end = time(NULL);
    printf("INFO! write read finish! packetlen=%d times=%d sendsum=%ld recvsum=%ld cost=%ld\n",
        sendlen, TEST_TIMES, sendsum, recvsum, end - start);

    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);
    return 0;
}

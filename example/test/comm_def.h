#ifndef COMM_DEF_H
#define COMM_DEF_H

#define SERVER_PORT     54321
#define MAX_DATA_LEN    512
#define MAX_EVENTS      8
#define BATCH           10
#define TEST_TIMES      1000
struct TestData {
    int seq {0};
    char buffer[MAX_DATA_LEN-4];
};
const char * SERVER_IP_eth0  = "10.50.12.133";
const char * SERVER_IP_eth1  = "10.50.12.75";

#endif

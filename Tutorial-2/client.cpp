/* 
    To run the client file:
    gcc client.c -o client
    To execute:
    ./client
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <thread>
#include <iostream>
#include <bits/stdc++.h>

using namespace std;

bool closeConnection = false;

void readFrmServer(int sockid) {
    char recvMsg[1024];
    while(!closeConnection) {
        bzero(recvMsg, 1024);
        int count = read(sockid, recvMsg, 1024);
        if(count < 0) {
            printf("Error on receiving message.\n");
        }
        printf("%s\n", recvMsg);
    }
}

void writeOnServer(int sockid) {
    char msg[1024];
    int count;
    while(!closeConnection) {
        bzero(msg, 1024);
        scanf("%s", msg);
        count = write(sockid, msg, strlen(msg));
        if(count < 0) {
            printf("Error on sending.\n");
        }
        if(strcmp(msg, "quit()") == 0) {
            closeConnection = true;
            close(sockid);
            exit(1);
        }
    }
}

int main(int argc, char const *argv[])
{
    int sockid;
    struct sockaddr_in addrport;
    struct hostent *server;

    sockid = socket(PF_INET, SOCK_STREAM, 0);
    if(sockid < 0) {
        printf("Socket could not be opened.\n");
    }

    if(server == NULL) {
        printf("Error, no such host.\n");
    }

    addrport.sin_family = AF_INET;
    addrport.sin_port = htons(3541);
    addrport.sin_addr.s_addr = inet_addr("127.0.0.1");

    if(connect(sockid, (struct sockaddr *) &addrport, sizeof(addrport)) < 0) {
        printf("Connection failed.\n");
    }

    thread readThread(readFrmServer, sockid);
    thread writeThread(writeOnServer, sockid);

    readThread.join();
    writeThread.join();
    
    return 0;
}
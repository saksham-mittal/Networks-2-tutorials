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

int main(int argc, char const *argv[])
{
    int sockid;
    struct sockaddr_in addrport, clientAddr;
    struct hostent *server;

    sockid = socket(PF_INET, SOCK_STREAM, 0);
    if(sockid < 0) {
        printf("Socket could not be opened.\n");
    }

    if(server == NULL) {
        printf("Error, no such host.\n");
    }

    addrport.sin_family = AF_INET;
    addrport.sin_port = htons(3542);
    addrport.sin_addr.s_addr = inet_addr("127.0.0.1");

    if(connect(sockid, (struct sockaddr *) &addrport, sizeof(addrport)) < 0) {
        printf("Connection failed.\n");
    }

    while(1) {
        char msg[1024], recvMsg[1024];
        bzero(recvMsg, 1024);
        int count = recv(sockid, recvMsg, sizeof(recvMsg), 0);
        if(count < 0) {
            printf("Error on receiving message.\n");
        }
        printf("User: %s\n", recvMsg);

        printf("Me: ");
        scanf("%s", msg);
        char* temp = "quit()";
        if(strcmp(msg, temp) == 0) {
            break;
        }
        count = send(sockid, msg, sizeof(msg), 0);
        if(count < 0) {
            printf("Error on sending.\n");
        }
    }
    close(sockid);
    return 0;
}
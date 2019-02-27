/* 
    To run the server file:
    gcc server.c -o server
    To execute:
    ./server
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

int main(int argc, char const *argv[])
{
    int sockid;
    struct sockaddr_in addrport, clientAddr;

    sockid = socket(PF_INET, SOCK_STREAM, 0);
    if(sockid < 0) {
        printf("Socket could not be opened.\n");
    }

    addrport.sin_family = AF_INET;
    addrport.sin_port = htons(3542);
    addrport.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if(bind(sockid, (struct sockaddr *) &addrport, sizeof(addrport))!= -1) {
        // Socket is opened
        int status = listen(sockid, 5);
        if(status < 0) {
            printf("Error in listening.\n");
        }
        socklen_t clilen = sizeof(clientAddr);
        int newsockid = accept(sockid, (struct sockaddr *)&clientAddr, &clilen);

        while(1) {
            char msg[1024], recvMsg[1024];
            printf("Me: ");
            scanf("%s", msg);
            char* temp = "quit()";
            if(strcmp(msg, temp) == 0) {
                break;
            }
            int count = send(newsockid, msg, sizeof(msg), 0);
            if(count < 0) {
                printf("Error on sending.\n");
            }

            bzero(recvMsg, 1024);
            count = recv(newsockid, recvMsg, sizeof(recvMsg), 0);
            if(count < 0) {
                printf("Error on receiving message.\n");
            }
            printf("User: %s\n", &recvMsg);
        }
        close(newsockid);
    } else {
        printf("Socket binding error.\n");
    }
    close(sockid);
    return 0;
}

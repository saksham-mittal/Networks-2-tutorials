#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <thread>
#include <strings.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include <iostream>
#include <chrono>

using namespace std;

struct sendNode {
    char functionName[1024];
    int numArgs;
    float args[100];
};

void send_msg(int sockid, struct sendNode* s) {
    // cout << "send msg called\n";
    int countSend = send(sockid, s, sizeof(* s), 0);
    if(countSend == -1) 
        printf("send failed\n"); 
    if(strcmp(s->functionName, "exit") == 0){
        close(sockid);
        exit(1);
    }
}
void recv_msg(int sockid) {
    // cout << "recvd msg called" << endl;
    char recvBuffer[2048];
    bzero(recvBuffer, 2048);
    int countRec = recv(sockid, recvBuffer, 2048 , 0);
    cout << "Received answer = " << recvBuffer << endl;
    if(countRec == -1) 
        printf("recieve failed\n");
    if(strcmp(recvBuffer, "exit") == 0){
        exit(1);
    }
}

int main() {
    while(1) {
        int sockid = socket(PF_INET, SOCK_STREAM, 0);
        char recvBuffer[1024];
        if(sockid < 0 )
            printf("connect failed client");
        struct sockaddr_in servAddr;
        struct sockaddr_in clientAddr;
        inet_pton(AF_INET, "127.0.0.1", &servAddr.sin_addr);
        servAddr.sin_family = AF_INET;
        servAddr.sin_port = htons(3542);

        struct sendNode *methodParams;
        methodParams = (struct sendNode*)malloc(sizeof(struct sendNode));
        
        cout << "Enter the function name : ";
        char s[1024];
        cin >> s;
        if(strcmp(s, "exit") == 0) {
            close(sockid);
            break;
        }
        strcpy(methodParams->functionName, s);
        
        cout << "Enter number of arguments : ";
        int numArgs;
        float j;
        cin >> numArgs;
        methodParams->numArgs = numArgs;

        cout << "Enter the arguments : ";
        
        for(int i=0; i<numArgs; i++) {
            cin >> j;
            methodParams->args[i] = j;
        }
        
        int connToServ = connect(sockid, (struct sockaddr *) &servAddr , sizeof(servAddr));
        if(connToServ < 0 )
            printf("failed to connect\n");
        send_msg(sockid, methodParams);
        recv_msg(sockid);
        close(sockid);

        cout << "Client going to sleep\n";
        this_thread::sleep_for(chrono::seconds(2));
    }

    return 0;
}
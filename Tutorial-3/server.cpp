#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <thread>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include <iostream>

int N;
int usersconnected = 0;
using namespace std;
vector<bool> openCon;
struct sockaddr_in addrport;
int sockid;
int servSize;
vector<int> acc;
int i_global;

void thread_func(int);

struct sendNode {
    char functionName[1024];
    int numArgs;
    float args[100];
};

void accept_thread() {
    i_global = 0;
    thread client[N + 100];
    
    while(1) {
        if(usersconnected < N) {
            acc.push_back(accept(sockid, (struct sockaddr *) &addrport, (socklen_t *) &servSize));
            if(acc[i_global] > 0) {
                openCon.push_back(true);
                usersconnected++;
                client[i_global] = thread(thread_func, i_global);
                i_global++;
            }
        }
        else if(usersconnected < 1)
            break;
    }
    for(int i = 0; i < usersconnected; i++) {
        client[i].join();
    }
}

void thread_func(int sender) {
    struct sendNode *recvBuffer;
    recvBuffer = (struct sendNode*)malloc(sizeof(struct sendNode));
    int countRec = recv(acc[sender], recvBuffer, sizeof(* recvBuffer) , 0);
    // cout << (recvBuffer->functionName) << endl;
    
    float result;
    string answer;
    if(strcmp(recvBuffer->functionName, "add") == 0) {
        result = 0;
        for(int i=0; i<recvBuffer->numArgs; i++) {
            result = result + recvBuffer->args[i];
        }
        answer = to_string(result);
    } else if (strcmp(recvBuffer->functionName, "cube") == 0) {
        result = recvBuffer->args[0] * recvBuffer->args[0] * recvBuffer->args[0];
        answer = to_string(result);
    } else if (strcmp(recvBuffer->functionName, "div") == 0) {
        int quotient, rem;
        quotient = (int)recvBuffer->args[0]/(int)recvBuffer->args[1];
        answer = to_string(quotient) + ",";
        rem = (int)recvBuffer->args[0]%(int)recvBuffer->args[1];
        answer += to_string(rem);
    }
    
    if(countRec == -1)
        printf("recieve failed");
    if(strcmp(recvBuffer->functionName, "exit") == 0 ) {
        openCon[sender] = false;
        usersconnected--;
        if(usersconnected < 1){
            close(acc[sender]);
            exit(1);
        } else
            close(acc[sender]);
    } else {
        int n = send(acc[sender], answer.c_str(), answer.length(), 0);
        if(n == -1)
            printf("Send failed server side");
    }     
    usersconnected--;
    close(acc[sender]);
}

int main() {
    N = 100;
    sockid = socket(PF_INET, SOCK_STREAM, 0);
    int t = 1;
    setsockopt(sockid, SOL_SOCKET,SO_REUSEADDR,&t,sizeof(int));
    addrport.sin_family = AF_INET;
    addrport.sin_port = htons(3542);
    addrport.sin_addr.s_addr  = htons(INADDR_ANY);
    if(bind(sockid, (struct sockaddr *) &addrport, sizeof(addrport)) < 0) {
        close(sockid);
        printf("bind failed");
    }
    int status = listen(sockid, N);

    if(status == -1)
    {
        close(sockid);
        printf("status error\n");
    }
    servSize = sizeof(addrport);
    accept_thread();

    for(int i = 0; i < N; i++) 
        close(acc[i]);
    close(sockid);
    return 0;
}
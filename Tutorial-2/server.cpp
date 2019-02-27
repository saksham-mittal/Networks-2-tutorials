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
#include <thread>
#include <iostream>
#include <bits/stdc++.h>

using namespace std;

vector<bool> closeConnection;
int numberOfOpenConnections;

vector<int> newsockid;

void connectUtilityMethod(int sender) {
    char msg[1024];
    int count;
    while(1) {
        bzero(msg, 1024);
        count = read(newsockid[sender], msg, 1024);
        cout << read << endl;
        if(count < 0) {
            printf("Error on sending.\n");
        }
        if(strncmp(msg, "quit()", 6) == 0) {
            close(newsockid[sender]);
            closeConnection[sender] = true;
            break;
        }

        for(int i=0; i<newsockid.size(); i++) {
            if(i != sender and closeConnection[i] == false) {
                count = write(newsockid[i], msg, strlen(msg));
            }
            if(count < 0) {
                printf("Error on sending.\n");
            }
        }
    }
}

void acceptNewClients() {
    int sockid;
    struct sockaddr_in addrport;

    sockid = socket(PF_INET, SOCK_STREAM, 0);
    if(sockid < 0) {
        printf("Socket could not be opened.\n");
    }

    addrport.sin_family = AF_INET;
    addrport.sin_port = htons(3541);
    addrport.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if(bind(sockid, (struct sockaddr *) &addrport, sizeof(addrport))!= -1) {
        // Socket is opened
        int status = listen(sockid, 5);
        if(status < 0) {
            printf("Error in listening.\n");
        }
        socklen_t clilen = sizeof(addrport);
        int n;
        cout << "Enter n : ";
        cin >> n;
        numberOfOpenConnections = n;

        int maxConn = 0;
        vector<thread> clients;
        
        while(maxConn < n) {
            newsockid.push_back(accept(sockid, (struct sockaddr *)&addrport, &clilen));
            closeConnection.push_back(false);
            if(newsockid[maxConn] < 0) {
                cout << "Error on accept\n";
            }

            clients.push_back(thread(connectUtilityMethod, maxConn));
            maxConn++;
        }

        for(int i=0; i<clients.size(); i++) {
            clients[i].join();
        }

        close(sockid);
    } else {
        printf("Socket binding error.\n");
    }
    close(sockid);
}

int main(int argc, char const *argv[])
{
    thread dynamicAccept(acceptNewClients);
    dynamicAccept.join();
    return 0;
}

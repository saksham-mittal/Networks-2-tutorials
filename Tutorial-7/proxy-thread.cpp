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

string extractHost(string buffer) {
    int len = buffer.find("\r") - buffer.find_last_of(" ") - 1;
    return buffer.substr(buffer.find_last_of(" ") + 1, len);
}

void downloadObject(int newsockid) {
    char buffer[1024];
    bzero(buffer, 1024);
    int count = recv(newsockid, buffer, sizeof(buffer), 0);
    if(count < 0) {
        printf("Error on receiving message.\n");
    }
    cout << "Request received by proxy from client" << endl;
    
    buffer[count] = '\0';
    // Converting buffer to string for easy string manipulation
    string content(buffer);
    cout << buffer << endl;

    // Extracting hostname from buffer
    string hostName = extractHost(buffer); 

    struct addrinfo addrport, *res;

    memset(&addrport, 0, sizeof addrport);
    addrport.ai_family = AF_INET;
    addrport.ai_socktype = SOCK_STREAM;
    getaddrinfo(hostName.c_str(), "80", &addrport, &res);

    int sockid = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if(sockid < 0) {
        printf("Socket could not be opened.\n");
    }

    if(connect(sockid, res->ai_addr, res->ai_addrlen) < 0)
        cout << "Could not connect to server";
    else
        cout << "Connection established\n";
    
    if(send(sockid, buffer, strlen(buffer), 0) < 0)
        cout << "Error with sending HEAD request\n";
    else {
        cout << "GET request sent to server from proxy\n";
        int numBytes = 0, size = 1;
        int c = 0;
        while(numBytes < size) {
            bzero(buffer, 1024);
            int bytesReceived = recv(sockid, buffer, sizeof(buffer), 0);
            if(bytesReceived <= 0) {
                cout << "Receive not successful" << endl;
            }
            buffer[bytesReceived] = '\0';
            char sendBuffer[bytesReceived];
            for(int i=0; i<bytesReceived; i++)
                sendBuffer[i] = buffer[i];
            count = send(newsockid, sendBuffer, sizeof(sendBuffer), 0);
            if(count < 0) {
                printf("Error on sending.\n");
            }

            c++;
            string string_data = string(buffer);
            numBytes += bytesReceived;

            if(c == 1) {
                // Converting buffer to string for easy string manipulation
                string content(buffer);

                int index = content.find("Content-Length: ") + 16;
                string sizeOfContent = content.substr(index , content.find("\n", index) - index);
                size = stoi(sizeOfContent);
                cout << size << endl;

                // For the first buffer, the meta data is also fetched
                int start = string_data.find("\r\n\r\n") + 4;                        
                // Subtracting the header size from the num of bytes received
                numBytes -= start;
            }
        }
    }

    close(newsockid);
}

int main(int argc, char const *argv[]) {
    struct sockaddr_in addrport, clientAddr;

    int sockid = socket(PF_INET, SOCK_STREAM, 0);
    if(sockid < 0) {
        printf("Socket could not be opened.\n");
    }

    addrport.sin_family = AF_INET;
    addrport.sin_port = htons(3542);
    addrport.sin_addr.s_addr = htonl(INADDR_ANY);
    int t = 1;
    setsockopt(sockid, SOL_SOCKET, SO_REUSEADDR, &t, sizeof(int));
    
    if(bind(sockid, (struct sockaddr *)&addrport, sizeof(addrport))!= -1) {
        // Socket is opened
        int status = listen(sockid, 1);
        if(status < 0) {
            printf("Error in listening.\n");
        }
        socklen_t clilen = sizeof(clientAddr);
        while(1) {
            int newsockid = accept(sockid, (struct sockaddr *)&clientAddr, &clilen);
            thread th(downloadObject, newsockid);
            th.detach();
        }
    }
    close(sockid);
    return 0;
}

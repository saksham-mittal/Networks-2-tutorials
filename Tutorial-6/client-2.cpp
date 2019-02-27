/* 
    To run the client file:
    gcc client.c -o client -std=c++11
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
#include <fstream>
#include <iostream>
#include <bits/stdc++.h>

using namespace std;

ofstream fileOut;

void receiveObject(int size, int sockid) {
    int numBytes = 0, c = 0, bytesReceived;
    char buffer[5000];

    while(numBytes < size) {
        bytesReceived = recv(sockid, buffer, sizeof(buffer), 0);
        if(bytesReceived < 0 ) {
            cout << "Recv failed" << endl;
            return;
        }

        c++;
        string string_data = string(buffer);
        numBytes += bytesReceived;

        if(c > 1) {
            for (int i=0; i < bytesReceived; i++)
                fileOut << buffer[i];
        } else {
            // For the first buffer, the meta data is also fetched
            int start = string_data.find("\r\n\r\n") + 4;
            for (int i=start; i < bytesReceived; i++)
                fileOut << buffer[i];
        }
    }
}

int main(int argc, char* argv[]) {
    vector<string> objectPaths;
    ifstream inFile;
    
    inFile.open("file-names.txt");
    if (!inFile) {
        cout << "Unable to open file";
        exit(1); // terminate with error
    }
    string objName;
    while(inFile >> objName)
        objectPaths.push_back(objName);
    
    struct addrinfo addrport, *res;

    memset(&addrport, 0, sizeof addrport);
    addrport.ai_family = AF_INET;
    addrport.ai_socktype = SOCK_STREAM;
    getaddrinfo("192.168.162.24", "80", &addrport, &res);

    for(int j=0; j<objectPaths.size(); j++) {
        int sockid = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if(sockid < 0) {
            printf("Socket could not be opened.\n");
        }

        if(connect(sockid, res->ai_addr, res->ai_addrlen) < 0)
            cout << "Could not connect to server";
        else
            cout << "Connection established\n";

        string objectPath = objectPaths[j];
        string headRequestPath = "HEAD " + objectPath + " HTTP/1.1\r\nHost: intranet.iith.ac.in\r\n\r\n";
        char *headRequest = (char *)headRequestPath.c_str();

        if(send(sockid, headRequest, strlen(headRequest), 0) < 0)
            cout << "Error with sending HEAD request\n";
        else {
            char buffer[5000];
            int numBytes = recv(sockid, buffer, sizeof(buffer) - 1, 0);
            if (numBytes < 0 )
                cout << "Receive not successfull" << endl;
            
            // Initialising with '\0' character
            buffer[numBytes] = '\0';
            // Converting buffer to string for easy string manipulation
            string content(buffer);

            // Finding the size of content using the content length feild
            int index = content.find("Content-Length: ") + 16;

            string sizeOfContent = content.substr(index , content.find("\n", index) - index);
            int size = stoi(sizeOfContent);
            cout << "Size of Content " << size << " bytes" << endl;

            // get file
            int ind;
            for(int i=objectPath.length() - 1; i>=0; i--) {
                if(objectPath[i] == '.') {
                    ind = i;
                    break;
                }
            }
            string fileName, extensionName;
            for(int i=ind; i>=0; i--) {
                if(objectPath[i] == '/') {
                    fileName = objectPath.substr(i + 1, ind - i - 1);
                    break;
                }
            }
            extensionName = objectPath.substr(ind + 1);
            string filePath = fileName + "." + extensionName;
            cout << "Writing to file " << filePath << endl;
            fileOut.open(filePath, ios::binary | ios::out);

            // GET request for the content
            string getRequestPath = "GET " + objectPath + " HTTP/1.1\r\nHost: intranet.iith.ac.in\r\n\r\n";
            char *getRequest = (char *)getRequestPath.c_str();
            if (send(sockid, getRequest, strlen(getRequest), 0) < 0)
                cout << "Error with sending GET request\n";

            receiveObject(size, sockid);
            fileOut.close();

            cout << "Writing to file complete" << endl;
        }
        close(sockid);
    }

    return 0;
}
/* 
    To run the client file:
    g++ client.cpp -o client
    To execute:
    ./client /files/home/IFS.pdf 192.168.162.24
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
#include <sys/stat.h> 
#include <chrono>
#include <sys/types.h> 
#include <bits/stdc++.h>

using namespace std;

string extractFileName(string buffer) {
    return buffer.substr(buffer.find_last_of("/") + 1);
}

int main(int argc, char const *argv[]) {
    if (argc != 3) {
		cout << "Correct usage <object-path> <hostname>";
		return 0;
	}
    string objectPath(argv[1]);
    string filePath = extractFileName(objectPath);
    string hostName(argv[2]);

    int sockid;
    struct sockaddr_in addrport, clientAddr;
    struct hostent *server;

    sockid = socket(PF_INET, SOCK_STREAM, 0);
    if(sockid < 0) {
        printf("Socket could not be opened.\n");
    }

    addrport.sin_family = AF_INET;
    addrport.sin_port = htons(3542);
    addrport.sin_addr.s_addr = inet_addr("127.0.0.1");

    if(connect(sockid, (struct sockaddr *) &addrport, sizeof(addrport)) < 0) {
        printf("Connection failed.\n");
    }
    
    string headRequestPath = "GET " + objectPath + " HTTP/1.1\r\nHost: " + hostName + "\r\n\r\n";
    char headRequest[headRequestPath.length() + 1];
    copy(headRequestPath.begin(), headRequestPath.end(), headRequest);
    headRequest[headRequestPath.length()] = '\0';

    // Time during request
    auto start = chrono::steady_clock::now();
    int count = send(sockid, headRequest, sizeof(headRequest), 0);
    if(count < 0) {
        printf("Error on sending.\n");
    }

    char buffer[1024];
    int numBytes = 0, size = 1, c = 0;
    ofstream fileOut;
    if(mkdir(hostName.c_str(), 0777) != -1) 
        cout << "Directory " << hostName << " created\n";

    filePath = hostName + "/" + filePath;
    fileOut.open(filePath, ios::binary | ios::out);

    while(numBytes < size) {
        bzero(buffer, 1024);
        int bytesReceived = recv(sockid, buffer, sizeof(buffer), 0);
        // cout << "Bytes received = " << bytesReceived << endl;
        if(bytesReceived <= 0) {
            cout << "Receive not successful" << endl;
        }

        c++;
        string string_data = string(buffer);
        numBytes += bytesReceived;

        if(c > 1) {
            for(int i=0; i<bytesReceived; i++)
                fileOut << buffer[i];
        } else {
            // Converting buffer to string for easy string manipulation
            string content(buffer);

            int index = content.find("Content-Length: ") + 16;
            string sizeOfContent = content.substr(index , content.find("\n", index) - index);
            size = stoi(sizeOfContent);

            // For the first buffer, the meta data is also fetched
            int start = string_data.find("\r\n\r\n") + 4;
            for(int i=start; i<bytesReceived; i++)
                fileOut << buffer[i];
            
            // Subtracting the header size from the num of bytes received
            numBytes -= start;
        }
    }
    fileOut.close();
    // Time during request finished
    auto end = chrono::steady_clock::now();
    int t = chrono::duration_cast<chrono::microseconds>(end - start).count();
    cout << "Time taken by proxy server to fullfill request = " << t << " microseconds\n";

    close(sockid);
    return 0;
}
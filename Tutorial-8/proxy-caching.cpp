/* 
    To run the server file:
    g++ proxy-caching.cpp -o proxy-caching
    To execute:
    ./proxy-caching
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h> 
#include <netinet/in.h>
#include <netdb.h>
#include <thread>
#include <fstream>
#include <iostream>
#include <bits/stdc++.h>

using namespace std;

/* Sample GET request:
GET /files/home/IFS.pdf HTTP/1.1\r\nHost: 192.168.162.24\r\n\r\n
 */
string extractHost(string buffer) {
    int len = buffer.find("\r\n\r\n") - buffer.find_last_of(" ") - 1;
    return buffer.substr(buffer.find_last_of(" ") + 1, len);
}

string extractFileName(string buffer) {
    string objectPath = buffer.substr(buffer.find(" ") + 1, buffer.find("HTTP/1.1") - buffer.find(" ") - 2);
    return objectPath.substr(objectPath.find_last_of("/") + 1);
}

int main(int argc, char const *argv[]) {
    vector<string> objectRequests;
    /* Keeps a count of how many times a website is accessed */
    map<string, int> hostCount;
    /* Keeps track whether the site's object uses cookies or not */
    map<string, bool> cookieMap;
    int indexVec = 0;

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
    
    if(bind(sockid, (struct sockaddr *)&addrport, sizeof(addrport)) != -1) {
        // Socket is opened
        while(true) {
            int status = listen(sockid, 1);
            if(status < 0) {
                printf("Error in listening.\n");
            }
            socklen_t clilen = sizeof(clientAddr);
            
            int newsockid = accept(sockid, (struct sockaddr *)&clientAddr, &clilen);

            char buffer[1024];
            bzero(buffer, 1024);
            int count = recv(newsockid, buffer, sizeof(buffer), 0);
            if(count < 0) {
                printf("Error on receiving message.\n");
            }
            cout << "Request received by proxy server from client" << endl;
            buffer[count] = '\0';
            // Converting buffer to string for easy string manipulation
            string content(buffer);
            // cout << content << endl;
            
            string fileName = extractFileName(buffer);
            /* Creates directory with name 'proxy' if it doesn't exist */
            if(mkdir("proxy", 0777) != -1) 
                cout << "Directory proxy created\n";

            fileName = "proxy/" + fileName;
            fstream fileStream;
            fileStream.open(fileName);

            // Extracting hostname from buffer
            string hostName = extractHost(buffer);

            if(hostCount.find(hostName) == hostCount.end())
                hostCount[hostName] = 1;
            else
                hostCount[hostName] += 1; 

            if(fileStream.fail()) {
                // Object not found
                cout << "Object not available with proxy server\n";
                struct addrinfo addrport, *res;

                memset(&addrport, 0, sizeof addrport);
                addrport.ai_family = AF_INET;
                addrport.ai_socktype = SOCK_STREAM;
                getaddrinfo(hostName.c_str(), "80", &addrport, &res);

                int sockidnew = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
                if(sockidnew < 0) {
                    printf("Socket could not be opened.\n");
                }

                cout << "Requesting object from the server\n";
                if(connect(sockidnew, res->ai_addr, res->ai_addrlen) < 0)
                    cout << "Could not connect to server";
                else
                    cout << "Connection established\n";
                
                if(send(sockidnew, buffer, strlen(buffer), 0) < 0)
                    cout << "Error with sending request\n";
                else {
                    cout << "GET request sent to server from proxy\n";
                    int numBytes = 0, size = 1;
                    int c = 0;
                    ofstream outFile;
                    cout << "Writing to file " + fileName + "\n";
                    outFile.open (fileName, ios::binary | ios::out);
                    indexVec++;

                    bool toBeCached = true;
                    while(numBytes < size) {
                        bzero(buffer, 1024);
                        int bytesReceived = recv(sockidnew, buffer, sizeof(buffer), 0);
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

                            if(content.find("Cache-Control") != string::npos) {
                                string valCache = content.substr(content.find("Cache-Control: ") + 15, 8);
                                if(valCache.compare("no-cache") == 0)
                                    toBeCached = false;
                            }

                            if(content.find("set-cookie") == string::npos)
                                cookieMap[hostName] = false;
                            else
                                cookieMap[hostName] = true;

                            // For the first buffer, the meta data is also fetched
                            int start = string_data.find("\r\n\r\n") + 4;                                      
                            // Subtracting the header size from the num of bytes received
                            numBytes -= start;
                        }
                        if(toBeCached == false) {
                            cout << "Content can't be cached by the proxy server\n";
                            continue;
                        }
                        for(int i=0; i<bytesReceived; i++)
                            outFile << buffer[i];
                    }
                    cout << "Wiriting to file completed\n";
                    cout << "Request finished by proxy server\n";
                    cout << "--------------------------------\n";
                    outFile.close();
                }
                close(sockidnew);
            } else {
                // File already cached
                cout << "File already cached by the proxy server\n";
                ifstream inFile, inFileDummy;
                inFileDummy.open(fileName, ios::binary | ios::ate);
                inFile.open(fileName, ios::binary | ios::out);
                int size = inFileDummy.tellg(), fileCount = 0;
                cout << "The size of the file is = " << size << " bytes\n";
            
                while(fileCount < size) {
                    int fz = 5000;
                    if(fileCount + 5000 > size)
                        fz = size - fileCount;
                    
                    fileCount += fz;
                    char dataSend[fz];
                    bzero(dataSend, fz);
                    inFile.read(dataSend, fz);
                    cout << "Sending " << sizeof(dataSend) << " bytes to the client\n";
                    send(newsockid, dataSend, sizeof(dataSend), 0);
                }
                cout << "File sending finished\n";
                cout << "Request finished by proxy server\n";
                cout << "--------------------------------\n";
            }
            bzero(buffer, 1024);
            close(newsockid);

            cout << "Web statistics of the proxy server: \n";
            cout << "Website Name | Number of times it is accessed\n";
            for(auto elem: hostCount) {
                cout << elem.first << " | " << elem.second << endl;
            }
            cout << "--------------------------------\n";
            cout << "Website Name | Uses cookie or not\n";
            for(auto elem: cookieMap) {
                cout << elem.first << " | " << elem.second << endl;
            }
            cout << "--------------------------------\n";
        }
    } else {
        printf("Socket binding error.\n");
    }
    close(sockid);
    cout << "Closing sockid\n";
    return 0;
}

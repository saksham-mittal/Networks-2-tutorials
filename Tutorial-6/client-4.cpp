#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <chrono>
#include <thread>
#include <fstream>
#include <iostream>
#include <mutex>
#include <bits/stdc++.h>

using namespace std;

#define NUMCONNECTIONS 6

FILE *fptr = fopen("logFile.txt", "w"); 

mutex mtx;
long long int counter = 0;

void downloadWesite(int thID, string hostName, vector<string> objPaths) {
    // To allocate objects to threads
    long long int j = 0;
    struct addrinfo addrport, *res;

    memset(&addrport, 0, sizeof addrport);
    addrport.ai_family = AF_INET;
    addrport.ai_socktype = SOCK_STREAM;
    getaddrinfo(hostName.c_str(), "80", &addrport, &res);

    int sockid = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if(sockid < 0) {
        fprintf(fptr, "Socket could not be opened.\n");
    }

    if(connect(sockid, res->ai_addr, res->ai_addrlen) < 0)
        cout << "Could not connect to server";
    else
        fprintf(fptr, "Connection established to server\n");

    bool retry = false;
    while(j < objPaths.size()) {
        if(!retry) {
            mtx.lock();
            j = counter;
            counter++;
            mtx.unlock();
        }

        if(j >= objPaths.size())
            break;

        if(!retry)
            fprintf(fptr, "Thread %d downloading the object %s\n", thID, (char*)objPaths[j].c_str());

        string objectPath = objPaths[j];
        string headRequestPath = "HEAD " + objectPath + " HTTP/1.1\r\nHost: " + hostName + "\r\n\r\n";
        char *headRequest = (char *)headRequestPath.c_str();
        string filePath;

        try
        {
            if(send(sockid, headRequest, strlen(headRequest), 0) < 0)
                cout << "Error with sending HEAD request\n";
            else {
                char buffer[5000];
                bzero(buffer, 5000);
                int numBytes = recv(sockid, buffer, sizeof(buffer) - 1, 0);
                if (numBytes <= 0 ) {
                    if(numBytes < 0)
                        cout << "Receive not successful" << endl;
                    throw(-1);      // Means exception in receiving
                }

                // Initialising with '\0' character
                buffer[numBytes] = '\0';
                // Converting buffer to string for easy string manipulation
                string content(buffer);

                // fprintf(fptr, "%s", buffer);

                // Finding the size of content using the content length feild in the HEAD request
                if(content.find("Content-Length: ") == string::npos) 
                    throw(-1);
                
                int index = content.find("Content-Length: ") + 16;

                string sizeOfContent = content.substr(index , content.find("\n", index) - index);
                int size = stoi(sizeOfContent);

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
                filePath = fileName + "." + extensionName;

                // GET request for the content
                string getRequestPath = "GET " + objectPath + " HTTP/1.1\r\nHost: " + hostName + "\r\n\r\n";
                char *getRequest = (char *)getRequestPath.c_str();
                if (send(sockid, getRequest, strlen(getRequest), 0) < 0)
                    cout << "Error with sending GET request\n";

                numBytes = 0;
                int c = 0, bytesReceived;
                ofstream fileOut;
                filePath = hostName + "/" + filePath;
                fileOut.open(filePath, ios::binary | ios::out);

                while(numBytes < size) {
                    bzero(buffer, 5000);
                    bytesReceived = recv(sockid, buffer, sizeof(buffer), 0);
                    if(bytesReceived <= 0) {
                        if(bytesReceived < 0)
                            cout << "Receive not successful" << endl;
                        throw(-1);      // Means exception in receiving
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
                        if(string_data.find("\r\n\r\n") == string::npos) 
                            throw(-1);
                        int start = string_data.find("\r\n\r\n") + 4;
                        for (int i=start; i < bytesReceived; i++)
                            fileOut << buffer[i];
                    }
                }
                fileOut.close();

                fprintf(fptr, "Writing to file %s completed by thread %d\n", (char*)filePath.c_str(), thID);
                retry = false;
            }
        }
        catch(...) {
            retry = true;
            fprintf(fptr, "Thread %d is retrying to download the object %s\n", thID, (char*)objPaths[j].c_str());
            continue;
        }
    }
    close(sockid);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
		cout << "Correct usage <hostName> <object-file>";
		return 0;
	}
	string hostName(argv[1]);
	string objectFileName(argv[2]);

    vector<string> objectPaths;
    ifstream inFile;
    
    inFile.open(objectFileName);
    if (!inFile) {
        cout << "Unable to open file";
        exit(1); // terminate with error
    }
    string objName;
    while(inFile >> objName)
        objectPaths.push_back(objName);

    // Initialising 6 TCP parallel connections
    thread tcpConns[NUMCONNECTIONS];

    auto begin = std::chrono::steady_clock::now();
    for(int i=0; i<NUMCONNECTIONS; i++) {
        tcpConns[i] = thread(downloadWesite, i + 1, hostName, objectPaths);
    }

    for(int i=0; i<6; i++) {
        tcpConns[i].join();
    }
    auto end = std::chrono::steady_clock::now();

    cout << "Time taken to download website = " << chrono::duration_cast<chrono::milliseconds>(end - begin).count() << " milliseconds" << endl;

    return 0;
}
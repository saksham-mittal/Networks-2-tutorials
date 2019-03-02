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
    if(!inFile) {
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
    getaddrinfo(hostName.c_str(), "80", &addrport, &res);

    int sockid = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if(sockid < 0) {
        printf("Socket could not be opened.\n");
    }

    if(connect(sockid, res->ai_addr, res->ai_addrlen) < 0)
        cout << "Could not connect to server";
    else
        cout << "Connection established\n";

    auto begin = std::chrono::steady_clock::now();
    for(int j=0; j<objectPaths.size(); j++) {
        printf("Downloading the object %s\n", (char*)objectPaths[j].c_str());

        string objectPath = objectPaths[j];
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
                    // if(numBytes < 0)
                    cout << "Receive not successful" << endl;
                    throw(-1);      // Means exception in receiving
                }

                // Initialising with '\0' character
                buffer[numBytes] = '\0';
                // Converting buffer to string for easy string manipulation
                string content(buffer);

                // Finding the size of content using the content length feild in the HEAD request
                if(content.find("Content-Length: ") == string::npos) 
                    throw(-1);
                
                int index = content.find("Content-Length: ") + 16;

                if(content.find("\n", index) == string::npos)
                    throw(-1);

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
                        // if(bytesReceived < 0)
                        cout << "Receive not successful" << endl;
                        throw(-1);      // Means exception in receiving
                    }

                    c++;
                    string string_data = string(buffer);
                    numBytes += bytesReceived;

                    if(c > 1) {
                        for (int i=0; i < bytesReceived; i++)
                            fileOut << buffer[i];
                    } else {
                        // For the first buffer, the meta data is also fetched
                        if(string_data.find("\r\n\r\n") == string::npos) {
                            throw(-1);
                        } 
                        int start = string_data.find("\r\n\r\n") + 4;
                        for (int i=start; i < bytesReceived; i++)
                            fileOut << buffer[i];
                        
                        // Subtracting the header size from the num of bytes received
                        numBytes -= start;
                    }
                }
                fileOut.close();

                printf("Writing to file %s completed\n", (char*)filePath.c_str());
            }
        }
        catch(...) {
            printf("Exception...\n");
        }
    }
    close(sockid);
    auto end = std::chrono::steady_clock::now();

    cout << "Time taken to download website = " << chrono::duration_cast<chrono::milliseconds>(end - begin).count() << " milliseconds" << endl;

    return 0;
}

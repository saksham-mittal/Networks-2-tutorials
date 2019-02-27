// Client side implementation of UDP Send and wait protocol
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h>
#include <sys/stat.h>       // For getting file size
#include <iostream>
#include <fstream>
#include <chrono>

using namespace std;

#define PORT            8080
#define MAXBUFFERSIZE   1024

// Header for the segment
struct headerSegment {
    int sequenceNumber;
    int acknowledgementNumber;
    int checkSum;
    int length;
    bool isLast;

    headerSegment() {
        sequenceNumber = 0;
        acknowledgementNumber = 1;
        checkSum = 0;
        length = MAXBUFFERSIZE;
        isLast = false;
    }
};

struct segment {
    struct headerSegment head;
    char payload[MAXBUFFERSIZE];
    
    segment() {
        for(int i=0; i<MAXBUFFERSIZE; i++)
            payload[i] = '\0';
    }
};

int calcCheckSum(headerSegment* packet, char* payload) {
    int ans = 0;
    ans ^= packet->sequenceNumber;
    ans ^= packet->acknowledgementNumber;
    ans ^= packet->isLast;
    ans ^= packet->length;
    
    if(payload) {
        // payload will be null if its an ack
        for(int i=0; i<MAXBUFFERSIZE; i++)
            ans ^= payload[i];
    }
}

bool compareChecksum(segment* packet) {
    return (packet->head.checkSum == calcCheckSum(&packet->head, packet->payload));
}

void sendUDPAck(int sockfd, segment* ackPacket, struct sockaddr* senderAddr) {
    ackPacket->head.checkSum = calcCheckSum(&ackPacket->head, ackPacket->payload);
    sendto(sockfd, ackPacket, sizeof(*ackPacket), 0, senderAddr, sizeof(*senderAddr));
}

int main(int argc, char **argv) {
    int sockfd;
    struct sockaddr_in servaddr;

    // Creates an unbound socket in the specified domain.
    // Returns socket file descriptor.
    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("Socket creation failed.\n");
        exit(1);
    }

    memset(&servaddr, 0, sizeof(servaddr));

    // Filling server information
    servaddr.sin_family = AF_INET;      // IPV4
    servaddr.sin_addr.s_addr = inet_addr("172.16.3.222");
    servaddr.sin_port = htons(PORT);

    // Assigns address to the unbound socket.
    if(bind(sockfd, (const struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        printf("Client Binding failed.\n");
        exit(1);
    }

    ofstream fileWrite;
    fileWrite.open("Received-File.txt", ios::binary | ios::in);

    segment* packet = new segment;  
    char buffer[MAXBUFFERSIZE];

    int sizeRead = 0;
    int currentAckNumber = 0, packetsReceived = 0;

    while(true) {
        struct sockaddr senderAddr;
        socklen_t serverlen = sizeof(senderAddr);

        while(true) {
            // Looping till we receive the packet from server
            if(recvfrom(sockfd, packet, sizeof(*packet), 0, &senderAddr, &serverlen) < 0) {
                cout << "Error in receiving UDP segment.\n";
            } else if(packet->head.sequenceNumber != currentAckNumber) {
                cout << "Packet " << currentAckNumber << " expected from the server\n";
                // Means the server has sent same segment
                segment* ackPacket = new segment;
                ackPacket->head.acknowledgementNumber = currentAckNumber;
                sendUDPAck(sockfd, ackPacket, &senderAddr);
            } else if(compareChecksum(packet) == false) {
                cout << "Incorrect checksum for UDP segment\n";
            } else {
                segment* ackPacket = new segment;
                ackPacket->head.acknowledgementNumber = currentAckNumber ^ 1;
                
                sendUDPAck(sockfd, ackPacket, &senderAddr);        
                packetsReceived++;
                break;
            }
        }
        
        int lengthPacket = packet->head.length;
        cout << "Received packet " << packet->head.sequenceNumber << " of length = " << lengthPacket << " bytes, checksum = " << packet->head.checkSum << endl;
    
        currentAckNumber = currentAckNumber ^ 1;

        if(packet->head.isLast) {
            cout << "Last packet received\n";
        }
        for(int i=0; i<lengthPacket; i++) {
            fileWrite << packet->payload[i];
        }
        if(packet->head.isLast)
            break;
    }
    cout << packetsReceived << " packets received" << endl;

    close(sockfd);
    return 0;
}
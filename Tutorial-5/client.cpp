// Client side implementation of Selective Repeat protocol
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h>
#include <sys/stat.h>       // For getting file size
#include <fstream>
#include <chrono>
#include <mutex>
#include <bits/stdc++.h>

using namespace std;

#define PORT            3454
#define MAXBUFFERSIZE   1024

// Header for the segment
struct headerSegment {
    int sequenceNumber;
    int acknowledgementNumber;
    int checkSum;
    int length;
    bool isLast;
    bool fynByte;

    headerSegment() {
        sequenceNumber = 0;
        acknowledgementNumber = 1;
        checkSum = 0;
        length = MAXBUFFERSIZE;
        isLast = false;
        fynByte = false;
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

// bool comparator method for sorting the buffered packets
bool cmpFunc(segment* s1, segment* s2) {
    return (s1->head.sequenceNumber < s2->head.sequenceNumber);
}

bool findSeqNumber(vector<segment*> vc, int seqNo) {
    for(int i=0; i<vc.size(); i++) {
        if(vc[i]->head.sequenceNumber == seqNo)
            return true;
    }
    return false;
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
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);

    // Assigns address to the unbound socket.
    if(bind(sockfd, (const struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        printf("Client Binding failed.\n");
        exit(1);
    }

    ofstream fileWrite;
    fileWrite.open("Received-File.txt", ios::binary | ios::in);

    char buffer[MAXBUFFERSIZE];

    int sizeRead = 0;
    int currentAckNumber = 0, packetsReceived = 0;

    struct sockaddr senderAddr;
    socklen_t serverlen = sizeof(senderAddr);
    
    vector<segment*> recvPackets;
    int numPackets = INT_MAX;

    while(true) {
        segment* packet = new segment;
        // Looping till we receive the packet from server
        if(recvfrom(sockfd, packet, sizeof(*packet), 0, &senderAddr, &serverlen) < 0) {
            cout << "Error in receiving UDP segment.\n";
        } else if(compareChecksum(packet) == false) {
            cout << "Incorrect checksum for UDP segment " << packet->head.sequenceNumber << "\n";
        } else {
            int lengthPacket = packet->head.length;
            if(findSeqNumber(recvPackets, packet->head.sequenceNumber))                
                cout << "Received duplicate packet " << packet->head.sequenceNumber << " of length = " << lengthPacket << " bytes, checksum = " << packet->head.checkSum << endl;
            else
                cout << "Received packet " << packet->head.sequenceNumber << " of length = " << lengthPacket << " bytes, checksum = " << packet->head.checkSum << endl;
        
            segment* ackPacket = new segment;
            ackPacket->head.acknowledgementNumber = packet->head.sequenceNumber;
            sendUDPAck(sockfd, ackPacket, &senderAddr);
        
            if(findSeqNumber(recvPackets, packet->head.sequenceNumber))                
                cout << "Sending ACK for duplicate packet " << packet->head.sequenceNumber << endl;
            else
                cout << "Sending ACK for packet " << packet->head.sequenceNumber << endl;
             
            // Buffer the packets(only if it doesn't contain a packet of the seq number of packet)
            if(!findSeqNumber(recvPackets, packet->head.sequenceNumber)) {
                recvPackets.push_back(packet);
                packetsReceived++;
                
                if(packet->head.isLast == true)
                    numPackets = packet->head.sequenceNumber + 1;
            }
        }
        // if(recvPackets.size() == numPackets) {
        if(packet->head.fynByte) {
            cout << "All packets received\n";
            cout << "Fyn packet received\n";
            break;
        }
    }
    cout << packetsReceived << " packets received" << endl;

    // Deliver the packets in order
    // i.e. sort the recvPackets vector
    sort(recvPackets.begin(), recvPackets.end(), cmpFunc);
    cout << "Started writing packets to file\n";
    for(int i=0; i<recvPackets.size(); i++) {
        // Iterating over all the packets
        for(int j=0; j<recvPackets[i]->head.length; j++) {
            fileWrite << recvPackets[i]->payload[j];
        }
    }

    close(sockfd);
    return 0;
}
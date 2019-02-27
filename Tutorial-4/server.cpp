// Server side implementation of UDP Send and wait protocol
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

#define PORTSERVER      8081
#define PORTCLIENT      8080
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

bool receiveACK(int sockfd, int ackNumber) {
    segment* ack = new segment;
    struct sockaddr clientAddr;
    socklen_t clientLength = sizeof(clientAddr);

    if(recvfrom(sockfd, ack, sizeof(*ack), 0, &clientAddr, &clientLength) < 0) {
        // ACK not received
        cout << "Error in receiving ACK\n";
        return false;
    } else if(ack->head.acknowledgementNumber != ackNumber) {
        // ACK received is for different packet
        cout << "ACK " << ackNumber << " expected for the UDP segment\n";
        return false;
    } else if(compareChecksum(ack) == false) {
        // Incorrect checksum
        cout << "Incorrect checksum for ACK\n";
        return false;
    }

    return true;
}

int main(int argc, char **argv) {
    if(argc != 2) {
        printf("usage: %s <file_path>\n", argv[0]);
        exit(1);
    }
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;

    // Creates an unbound socket in the specified domain.
    // Returns socket file descriptor.
    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("Socket creation failed.\n");
        exit(1);
    }

    struct timeval timeValue;
	timeValue.tv_sec = 0;
	timeValue.tv_usec = 50000;
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeValue, sizeof(timeValue)) < 0) {
		cout << "Error in creating timeout for UDP connection " << endl;
		return 0;
	}

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    // Filling server information
    servaddr.sin_family = AF_INET;      // IPV4
    servaddr.sin_addr.s_addr = inet_addr("172.16.3.222");
    servaddr.sin_port = htons(PORTSERVER);

    // Filling client information
    cliaddr.sin_family = AF_INET;      // IPV4
    cliaddr.sin_addr.s_addr = inet_addr("172.16.3.222");
    cliaddr.sin_port = htons(PORTCLIENT);

    // Assigns address to the unbound socket.
    if(bind(sockfd, (const struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        printf("Binding failed.\n");
        exit(1);
    }

    struct stat st;
    stat(argv[1], &st);
    unsigned int fileSize = st.st_size;
    printf("Sending file of size = %u\n", fileSize);

    ifstream fileRead;
    fileRead.open(argv[1], ios::binary | ios::in);

    segment* packet = new segment;  
    char buffer[MAXBUFFERSIZE];

    int sizeRead = 0;
    int currentSequenceNumber = 0, currentAckNumber = 1;
    int numberPacketsSent = 0;

    auto begin = std::chrono::steady_clock::now();

    while(fileRead.eof() == false) {
        // Filling the packet
        fileRead.read(packet->payload, MAXBUFFERSIZE);
        packet->head.sequenceNumber = currentSequenceNumber;
        packet->head.acknowledgementNumber = currentAckNumber;
        
        // Updating the size of file read so far
        sizeRead += MAXBUFFERSIZE;

        // Checking if we are sending the last packet
        if(sizeRead > fileSize) {
            packet->head.isLast = true;
            packet->head.length += fileSize - sizeRead; 
        } 

        packet->head.checkSum = calcCheckSum(&packet->head, packet->payload);
        while(true) {
            if(sendto(sockfd, packet, sizeof(*packet), 0, (const struct sockaddr*)&cliaddr, sizeof(cliaddr)) < 0) {
                // Send failed
                printf("Error in sending UDP segment.\n");
            } else {
                // Packet sent successfully
                numberPacketsSent++;
                if(receiveACK(sockfd, currentAckNumber)) {
                    // ACK received with correct sequence number
                    break;
                }
                cout << "Time out\nTrying to send packet again....\n";
            }
        }
        cout << "Sending packet " << packet->head.sequenceNumber << " of length = " << packet->head.length << " bytes, checksum = " << packet->head.checkSum << endl;
        if(packet->head.isLast) {
            cout << "Last packet sent\n";
        }

        // Updating the seq and ack number 
        currentSequenceNumber = currentSequenceNumber ^ 1;
        currentAckNumber = currentAckNumber ^ 1;
    }
    auto end = std::chrono::steady_clock::now();
    cout << numberPacketsSent << " packets sent.\n";

    cout << "Time taken for transfer = " << chrono::duration_cast<chrono::milliseconds>(end - begin).count() << " milliseconds" << endl;
    close(sockfd);
    return 0;
}
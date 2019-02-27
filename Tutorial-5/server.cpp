// Server side implementation of Selective Repeat protocol
// Command to add delay
// sudo tc qdisc add dev lo root netem delay 20ms
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
#include <bits/stdc++.h>
#include <thread>
#include <mutex>

using namespace std;

#define PORTSERVER      3458
#define PORTCLIENT      3454
#define MAXBUFFERSIZE   1024
#define WINDOWSIZE      20

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
        acknowledgementNumber = 0;
        checkSum = 0;
        length = MAXBUFFERSIZE;
        isLast = false;
        fynByte = false;
    }
};

bool* ackedPackets;
mutex mtx;

struct segment {
    struct headerSegment head;
    char payload[MAXBUFFERSIZE];
    
    segment() {
        for(int i=0; i<MAXBUFFERSIZE; i++)
            payload[i] = '\0';
    }
};

// Vector for storing the packets to be sent
vector<segment*> vcPacket;
int numAcks = 0;

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

int *numRetries;
void receiveACK(int sockfd, int* startSeq, int *currentSeq, int nPackets) {
    struct sockaddr clientAddr;
    socklen_t clientLength = sizeof(clientAddr);

    while(true) {
        segment* ack = new segment;

        int flagInt = 0;
        for(int i=0; i<nPackets; i++) {
            if(numRetries[i] == 30 || ackedPackets[i] == true) {
                flagInt++;
            }
        }
        if(flagInt == nPackets) {
            *startSeq = nPackets;
            break;
        }

        // Loop till all the packets are received
        if(recvfrom(sockfd, ack, sizeof(*ack), 0, &clientAddr, &clientLength) < 0) {
            // Time out condition
            // ACK not received
            printf("Error in receiving ACK\n");
        } else if(compareChecksum(ack) == false) {
            // Incorrect checksum
            printf("Incorrect checksum for ACK\n");
        } else {
            // Making the ack number index as true
            if(ackedPackets[ack->head.acknowledgementNumber] == false)
                numAcks++;
            ackedPackets[ack->head.acknowledgementNumber] = true;
            printf("Ack received for packet %d\n", ack->head.acknowledgementNumber);
            
            // Changing the start sequence number of window (if possible)
            int newSeqStart = *startSeq;
            for(int i=*startSeq; i<*currentSeq; i++) {
                if(ackedPackets[i] == true)
                    newSeqStart++;
                else
                    break;
            }
            *startSeq = newSeqStart;
            // if(*startSeq > (*nPackets - 2)) 
            //     break;
        }
    }
}

void callTimer(int sockfd, int currentSequenceNumber, struct sockaddr_in *cliaddr) {
    segment* packet2 = new segment;
    packet2 = vcPacket[currentSequenceNumber];
    printf("Sending packet %d of length = %d bytes, checksum = %d\n", packet2->head.sequenceNumber, packet2->head.length, packet2->head.checkSum);
    if(sendto(sockfd, packet2, sizeof(*packet2), 0, (const struct sockaddr*)cliaddr, sizeof(*cliaddr)) < 0) {
        // Send failed
        printf("Error in sending UDP segment.\n");
    }

    while(true && numRetries[currentSequenceNumber] < 30) {
        if(ackedPackets[currentSequenceNumber] == true)
            break;
        
        // sleep for timeout
        this_thread::sleep_for(chrono::milliseconds(50));

        // if timeout and ack not received, then resend
        if(ackedPackets[currentSequenceNumber] == false) {
            printf("Resending packet %d of length = %d bytes, checksum = %d\n", packet2->head.sequenceNumber, packet2->head.length, packet2->head.checkSum);
            if(sendto(sockfd, packet2, sizeof(*packet2), 0, (const struct sockaddr*)cliaddr, sizeof(*cliaddr)) < 0) {
                // Send failed
                printf("Error in sending UDP segment.\n");
            }
            numRetries[currentSequenceNumber]++;
        }
    }
    if(numRetries[currentSequenceNumber] == 30) {
        printf("Couldn't send packet %d\n", currentSequenceNumber);
    }
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
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORTSERVER);

    // Filling client information
    cliaddr.sin_family = AF_INET;      // IPV4
    cliaddr.sin_addr.s_addr = INADDR_ANY;
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

    char buffer[MAXBUFFERSIZE];

    int sizeRead = 0;
    int nPackets = 0, currentSequenceNumber = 0;

    auto begin = std::chrono::steady_clock::now();

    while(fileRead.eof() == false) {
        segment* packet = new segment;
        // Filling the packet
        fileRead.read(packet->payload, MAXBUFFERSIZE);
        packet->head.sequenceNumber = currentSequenceNumber;

        // Updating the size of file read so far
        sizeRead += MAXBUFFERSIZE;

        // Checking if we are sending the last packet
        if(sizeRead > fileSize) {
            packet->head.isLast = true;
            packet->head.length += fileSize - sizeRead; 
        } 

        packet->head.checkSum = calcCheckSum(&packet->head, packet->payload);
        vcPacket.push_back(packet);
        nPackets++;
        currentSequenceNumber++;
    }

    currentSequenceNumber = 0;

    ackedPackets = new bool[nPackets];
    numRetries = new int[nPackets];
    for(int i=0; i<nPackets; i++) {
        ackedPackets[i] = false;
        numRetries[i] = 0;
    }

    int startWindowSeq = 0;
    thread receiverThread = thread(receiveACK, sockfd, &startWindowSeq, &currentSequenceNumber, nPackets);
    
    // Timer threads for handling timeout
    thread thTimer;
    while(startWindowSeq < nPackets - 1) {

        // We send all the packets which lie in the window together, and our receiver
        // thread waits for the acks from the client
        if(currentSequenceNumber < startWindowSeq + WINDOWSIZE and currentSequenceNumber < nPackets) {
            // The current sequence number is in the window
            // Timer thread
            thTimer = thread(callTimer, sockfd, currentSequenceNumber, &cliaddr);
            thTimer.detach();
            currentSequenceNumber++;
        }
    }

    receiverThread.join();
    
    segment* packetFyn = new segment;
    packetFyn->head.fynByte = true;

    if(sendto(sockfd, packetFyn, sizeof(*packetFyn), 0, (const struct sockaddr*)&cliaddr, sizeof(cliaddr)) < 0) {
        // Send failed
        printf("Error in sending UDP segment.\n");
    } else
        printf("Fyn packet sent.\n");

    auto end = std::chrono::steady_clock::now();
    cout << nPackets << " packets sent.\n";

    cout << "Time taken for transfer = " << chrono::duration_cast<chrono::milliseconds>(end - begin).count() << " milliseconds" << endl;
    close(sockfd);
    return 0;
}
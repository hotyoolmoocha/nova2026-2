#ifndef PACKET_H
#define PACKET_H

#define MAX_PACKET 100

typedef struct
{
    int seqNum;         // 패킷 번호
    int acknowledged;   // ACK 수신 여부
    int transmitted;    // 전송 여부
    int lost;           // 손실 여부
    int retransmitted;  // 재전송 여부
} Packet;

void initializePackets(Packet packets[], int count);

#endif
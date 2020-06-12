#ifndef MAINH
#define MAINH

#include <iostream>
// #include <cstdlib>
#include <mpi.h>
#include <unistd.h>
#include <pthread.h>

using namespace std;


#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"

#define MAX_ROOM 10
#define MAX_ELEV 5
class Packet { 
public:
    int ts; // lamport clock timestamp 
    int src; // sender tid, probably unnecessary 
    int data; 
};

enum State { // in proper order
    INIT,
    WAIT_ACK_ROOM,
    WAIT_ROOM,
    WAIT_ACK_ELEV,
    WAIT_ELEV,
    IN_ELEV,
    IN_ROOM,
    WAIT_ACK_ELEV_BACK,
    WAIT_ELEV_BACK,
    IN_ELEV_BACK,
    END
};

enum Message {
    ACK, 
    REQ_ELEV, 
    REQ_ROOM, 
    REL_ELEV, 
    REL_ROOM, 
    FINISH
};

extern MPI_Datatype MPI_PACKET_T;
extern State state;

void changeState(State);
void ackReceived(int);
void changeResources(int, Packet);

#endif 
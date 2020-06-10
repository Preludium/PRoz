#include "main.h"
#include "process.h"
#include "communicationThread.h"
#include <string>

MPI_Datatype MPI_PACKET_T;
MPI_Status status;

int size, tid;
pthread_t threadCom;
State state = INIT;
pthread_mutex_t stateMut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t resourceMut = PTHREAD_MUTEX_INITIALIZER;
Process process;

// pROBABLy need to add check_thread_support from "magazyn" but dunno what it does
// we can skip WAIT_ACK states but need to check AckCounter in changeResource() for WAIT_[...] state
// dunno if its worth it

void init();
void mainLoop();
void finalize();
void sendPacket(Packet*, int, int);
void changeState();
void ackReceived();
void changeResources();
void askForResource(Packet*, int);
void printDebugInfo(string);
string getResourceString(int);

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    init();
    mainLoop();
    finalize();
    return 0;
}

void init() {
    const int nitems = 3;
    int blocklengths[3] = { 1, 1, 1 };
    MPI_Datatype types[3] = { MPI_INT, MPI_INT, MPI_INT };

    MPI_Aint offsets[3]; 
    offsets[0] = offsetof(Packet, ts);
    offsets[1] = offsetof(Packet, src);
    offsets[2] = offsetof(Packet, data);

    MPI_Type_create_struct(nitems, blocklengths, offsets, types, &MPI_PACKET_T);
    MPI_Type_commit(&MPI_PACKET_T);

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &tid);
    srand(tid);

    pthread_create( &threadCom, NULL, startCommunicationThread , 0);
}

void mainLoop() {
    process = Process();
    Packet *packet = new Packet();
    packet->data = process.getTrashes();

    askForResource(packet, Message::REQ_ROOM);
    changeState(State::WAIT_ACK_ROOM);
    // wait for all ACKs somehow xD
    changeState(State::WAIT_ROOM);
    // check room resource
    // decrease room - RESOURCE MUTEX
    changeState(State::WAIT_ACK_ELEV);



    // next: get elevator 
    // wait for resources
    // go down in elevator
    // RES_ELEV
    // go to room
    // trash out
    // get the elevator
    // RES_ROOM
    // go up in elevator
    // RES_ELEV
    // FINISH
}

void finalize() {
    //cout << "Waiting for communication thread" << endl;
    pthread_join(threadCom, NULL);
    MPI_Type_free(&MPI_PACKET_T);
    MPI_Finalize();
}

void sendPacket(Packet *packet, int destination, int tag) {
    int freePacket = false;
    if (packet == 0) {
        packet = new Packet();
        freePacket = true;
    }
    pthread_mutex_lock( &resourceMut );
    process.incrementTimeStamp(); // mutegz 
    pthread_mutex_unlock( &resourceMut );
    packet->src = tid;
    packet->ts = process.getTimeStamp();
    MPI_Send(packet, 1, MPI_PACKET_T, destination, tag, MPI_COMM_WORLD);
    if (freePacket) 
        free(packet);
}

void changeState( State newState ) {
    pthread_mutex_lock( &stateMut );
    if (state == END) { 
	    pthread_mutex_unlock( &stateMut );
        return;
    }
    state = newState;
    pthread_mutex_unlock( &stateMut );
}

void ackReceived() {
    pthread_mutex_lock( &resourceMut );
    process.incrementAckCounter(); // probably lock mutegz
    pthread_mutex_unlock( &resourceMut );
    if (process.getAckCounter() == size - 1);
	    pthread_mutex_unlock( &stateMut );
}

void setTimeStamp(int ts) { // DO NOT USE IF NOT IN RESOURCE MUTEX AREA
    process.setTimeStamp(max(ts, process.getTimeStamp()) + 1);
}

void sendAck(int destination) {
    Packet *packet = new Packet();
    packet->src = tid;
    packet->ts = process.getTimeStamp();
    MPI_Send( packet, 1, MPI_PACKET_T, destination, Message::ACK, MPI_COMM_WORLD);
}

void askForResource(Packet *packet, int tag) {
    printDebugInfo("Składam żądania o " + getResourceString(tag));
    for (int i = 0; i < size; ++i)
        if (i != tid) 
            sendPacket(packet, i, tag);
}

void printDebugInfo(string msg) {
    cout << "[" << tid << "] " << "[" << process.getTimeStamp() << "] ";
    cout << msg << endl;
}

string getResourceString(int resTag) {
    string out = "";
    if (resTag == Message::REQ_ROOM || resTag == Message::RES_ROOM) 
        out = "pokoje";
    else if (resTag == Message::REQ_ELEV || resTag == Message::RES_ELEV) 
        out = "windy";
    return out;
}

void changeResources(int msg, Packet packet) {
    pthread_mutex_lock( &resourceMut );
    int ts = packet.ts, data = packet.data, src = packet.src;

    switch (msg) {
        case Message::REQ_ROOM:
            switch (state) {
                case INIT:
                case IN_ELEV_BACK:
                    process.decreaseHeadRoom(data);
                    break;
                case WAIT_ACK_ROOM:
                    if (process.getTimeStamp() < ts) // it means that incoming packet is in front of this process in Q
                        process.decreaseHeadRoom(data);
                    else
                        process.increaseTailRoom(data);
                    break;
                case WAIT_ROOM:
                case WAIT_ACK_ELEV:
                case WAIT_ELEV:
                case IN_ELEV:
                case IN_ROOM:
                case WAIT_ACK_ELEV_BACK:
                case WAIT_ELEV_BACK:
                    process.increaseTailRoom(data);
                    break;
            }
            break;

        case Message::REQ_ELEV:
            switch (state) {
                case INIT:
                case WAIT_ACK_ROOM:
                case WAIT_ROOM:
                case IN_ROOM:
                    process.decrementHeadElev();
                    break;
                case WAIT_ACK_ELEV:
                case WAIT_ACK_ELEV_BACK:
                    if (process.getTimeStamp() < ts) // it means that incoming packet is in front of this process in Q
                        process.decrementHeadElev();
                    else
                        process.incrementTailElev();
                    break;
                case WAIT_ELEV:
                case IN_ELEV:
                case WAIT_ELEV_BACK:
                case IN_ELEV_BACK:
                    process.incrementTailElev();
                    break;
            }
            break;

        case Message::RES_ROOM:
            if (state == WAIT_ACK_ROOM) {
                process.increaseHeadRoom(data);
                // unlock mutex and proceed to state WAIT_ACK_ELEV
                pthread_mutex_unlock( &stateMut );
            } else 
                process.increaseHeadRoom(data);
            break;

        case Message::RES_ELEV:
            if (state == WAIT_ELEV) {
                process.incrementHeadElev();
                // unlock mutex and proceed to state IN_ELEV
                pthread_mutex_unlock( &stateMut );
            } else if (state == WAIT_ELEV_BACK) {
                process.incrementHeadElev();
                // unlock mutex and proceed to state IN_ELEV_BACK
                pthread_mutex_unlock( &stateMut );
            } else 
                process.incrementHeadElev();
            break;
    }
    setTimeStamp(ts);
    pthread_mutex_unlock( &resourceMut );
    sendAck(src);
}
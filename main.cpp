#include "main.h"
#include "process.h"
#include "communicationThread.h"
#include <string>

MPI_Datatype MPI_PACKET_T;
MPI_Status status;

int size, tid;
bool canProceed = 0;
pthread_t threadCom;
State state = INIT;
pthread_mutex_t stateMut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t resourceMut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t ackMut = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t ackCond = PTHREAD_COND_INITIALIZER;
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
void sendToAll(Packet*, int);
void printDebugInfo(string);
string getResourceString(int);
void threadWait();
void threadWake();
void setAfterAckReceived(int&, State&);
void requestResource(State, Message, string, Packet*);
int randTime(int);

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

    // now we are sending request and release to everyone (including myself), is it ok??
    
    // here: first we change state to WAIT_ACK_ROOM, then we check if we received enough
    // acks, then we check if we have enough rooms, if yes -> wake, no -> change state to WAIT_ROOM
    requestResource(State::WAIT_ACK_ROOM, Message::REQ_ROOM, "Żądam pokoi", packet);
    requestResource(State::WAIT_ACK_ELEV, Message::REQ_ELEV, "Żądam wind na dół", packet);
    
    changeState(State::IN_ELEV);
    printDebugInfo(">SK< Jadę windą na dół");
    sleep(randTime(5));
    printDebugInfo("<SK> Zjechałem na dół");


    printDebugInfo("Zwalniam zasób windy (na dole)");
    sendToAll(packet, Message::REL_ELEV);


    changeState(State::IN_ROOM);
    printDebugInfo(">SK< Zajmuję pomieszczenie i wyrzucam śmieci");
    sleep(randTime(5));
    printDebugInfo("<SK> Wyrzuciłem śmieci");

    requestResource(State::WAIT_ACK_ELEV_BACK, Message::REQ_ELEV, "Żądam wind na górę", packet);
    
    printDebugInfo("Zwalniam zasób pokoje");
    sendToAll(packet, Message::REL_ROOM);


    changeState(State::IN_ELEV_BACK);
    printDebugInfo(">SK< Wjeżdżam windą na górę");
    sleep(randTime(5));
    printDebugInfo("<SK> Wjechałem na górę");

    printDebugInfo("Zwalniam zasób windy (na górze)");
    sendToAll(packet, Message::REL_ELEV);

    // FINISH
}

void finalize() {
    //cout << "Waiting for communication thread" << endl;
    pthread_join(threadCom, NULL);
    MPI_Type_free(&MPI_PACKET_T);
    MPI_Finalize();
}

void requestResource(State ackState, Message tag, string debugInfo, Packet* packet) {
    printDebugInfo(debugInfo);
    sendToAll(packet, tag);
    changeState(ackState);
    pthread_mutex_lock(&ackMut);
    if (!canProceed)
        threadWait();
    canProceed = 0;
    pthread_mutex_unlock(&ackMut);
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
    if (process.getAckCounter() == size - 1) {
        process.setAckCounter(0);
        int resource;
        State newState;
        setAfterAckReceived(resource, newState);
        if (resource >= 0)
            threadWake();
        else
            changeState(newState);
    }
}

void setAfterAckReceived(int &resource, State &newState) {
    if (state == State::WAIT_ACK_ELEV || state == State::WAIT_ACK_ELEV_BACK) {
        resource = process.getHeadElev();
        if (state == State::WAIT_ACK_ELEV)
            newState = State::WAIT_ELEV;
        else
            newState = State::WAIT_ELEV_BACK;
    } else if (state == State::WAIT_ACK_ROOM) {
        resource = process.getHeadRoom();
        newState = State::WAIT_ROOM;
    }
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


void sendToAll(Packet *packet, int tag) {
    for (int i = 0; i < size; ++i) 
        sendPacket(packet, i, tag);
}


void printDebugInfo(string msg) {
    cout << "[" << tid << "] " << "[" << process.getTimeStamp() << "] ";
    cout << msg << endl;
}

string getResourceString(int resTag) {
    string out = "";
    if (resTag == Message::REQ_ROOM || resTag == Message::REL_ROOM) 
        out = "pokoje";
    else if (resTag == Message::REQ_ELEV || resTag == Message::REL_ELEV) 
        out = "windy";
    return out;
}

void threadWait() {
    pthread_cond_wait(&ackCond, &ackMut);
}

void threadWake() {
    pthread_mutex_lock(&ackMut);
    canProceed = 1;
    pthread_cond_signal(&ackCond);
    pthread_mutex_unlock(&ackMut);
}

int randTime(int max) {
    return rand() % max + 1;
}

void changeResources(int msg, Packet packet) {
    pthread_mutex_lock( &resourceMut );
    int ts = packet.ts, data = packet.data, src = packet.src;

    switch (msg) {
        case Message::REQ_ROOM:
            sendAck(src);
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
            sendAck(src);
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

        case Message::REL_ROOM:
            if (state == WAIT_ROOM) {
                process.increaseHeadRoom(data);
                if (process.getHeadRoom() >= 0)
                    threadWake();
            } else 
                process.increaseHeadRoom(data);
            break;

        case Message::REL_ELEV:
            if (state == WAIT_ELEV || state == WAIT_ELEV_BACK) {
                process.incrementHeadElev();
                threadWake();
            } else 
                process.incrementHeadElev();
            break;
    }
    setTimeStamp(ts);
    pthread_mutex_unlock( &resourceMut );
}
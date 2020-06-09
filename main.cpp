#include "main.h"
#include "process.h"
#include "communicationThread.h"

MPI_Datatype MPI_PACKET_T;
MPI_Status status;

int size, tid;
pthread_t threadCom;
state_t state = Running;
pthread_mutex_t stateMut = PTHREAD_MUTEX_INITIALIZER;
Process process;

void init();
void mainLoop();
void finalize();
void sendPacket();
void changeState();
void ackReceived();
void changeResources();

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    init();
    mainLoop();
    finalize();
    return 0;
}

void init() {
    const int nitems=3;
    int blocklengths[3] = {1,1,1};
    MPI_Datatype types[3] = {MPI_INT, MPI_INT, MPI_INT};

    MPI_Aint offsets[3]; 
    offsets[0] = offsetof(packet_t, ts);
    offsets[1] = offsetof(packet_t, src);
    offsets[2] = offsetof(packet_t, data);

    MPI_Type_create_struct(nitems, blocklengths, offsets, types, &MPI_PACKET_T);
    MPI_Type_commit(&MPI_PACKET_T);

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &tid);
    srand(tid);

    pthread_create( &threadCom, NULL, startCommunicationThread , 0);
}

void mainLoop() {
    process = Process();
    packet_t *packet = static_cast<packet_t*>(malloc(sizeof(packet_t))); 
    packet->data = process.getTrashes();
    sendPacket(packet, Message::REQ_ROOM);

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
    cout << "Waiting for communication thread" << endl;
    pthread_join(threadCom, NULL);
    MPI_Type_free(&MPI_PACKET_T);
    MPI_Finalize();
}

void sendPacket(packet_t *packet, int tag, int destination = MPI_ANY_SOURCE)
{

    int freePacket = false;
    if (packet == 0) {
        packet = static_cast<packet_t*>(malloc(sizeof(packet_t))); 
        freePacket = true;
    }

    process.incrementTimeStamp();
    packet->src = tid;
    packet->ts = process.getTimeStamp();
    MPI_Send( packet, 1, MPI_PACKET_T, destination, tag, MPI_COMM_WORLD);
    if (freePacket) 
        free(packet);
}

void changeState( state_t newState )
{
    pthread_mutex_lock( &stateMut );
    if (state == Finish) { 
	    pthread_mutex_unlock( &stateMut );
        return;
    }
    state = newState;
    pthread_mutex_unlock( &stateMut );
}

void ackReceived() {
    // increment process ack counter
    // if ack counter == num of processes - 1 then 
    // unlock proper mutex

    process.incrementAckCounter();
    if (process.getAckCounter() == size - 1); // make sure if "size" is number of processes
        // unlockmutegz 
}

void setTimeStamp(int ts) {
    process.setTimeStamp(max(ts, process.getTimeStamp()) + 1);
}

void changeResources(int msg, int data, int ts) {
    switch (msg) {
        case Message::REQ_ELEV:
            if (process.getTimeStamp() < ts) // it means that incoming packet is in front of this process in Q
                process.decrementHeadElev();
            else
                process.incrementTailElev();

            setTimeStamp(ts);
            break;

        case Message::REQ_ROOM:
            if (process.getTimeStamp() < ts)
                process.decreaseHeadRoom(data);
            else
                process.increaseTailRoom(data);

            setTimeStamp(ts);
            break;

        case Message::RES_ELEV:
            process.incrementHeadElev();
            setTimeStamp(ts);
            break;

        case Message::RES_ROOM:
            process.increaseHeadRoom(data);
            setTimeStamp(ts);
            break;
        
        default:
            break;
    }
}
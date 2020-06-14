#include "main.h"
#include "process.h"
#include "communicationThread.h"
#include <string>

MPI_Datatype MPI_PACKET_T;
MPI_Status status;
const bool debug = false;

int size, tid;
pthread_t threadCom;
State state = INIT;
pthread_mutex_t stateMut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t resourceMut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t ackMut = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t ackCond = PTHREAD_COND_INITIALIZER;
Process process;

void init();
void mainLoop();
void finalize();
void reduceRoom(int);
void reduceElev();
void beforeReleaseElev();
void beforeReleaseRoom(int);
void sendPacket(Packet*, int, int);
void changeState();
void ackReceived();
void changeResources();
void sendToAll(Packet*, int);
void printDebugInfo(string, string = "");
string getResourceString(int);
void threadWait();
void threadWake();
void setTimeStamp(int);
void setAfterAckReceived(int&, State&);
void requestResource(State, Message, string, Packet*);
int randTime(int);

void check_thread_support(int provided) {
    if (debug) printf("THREAD SUPPORT: chcemy %d. Co otrzymamy?\n", provided);
    switch (provided) {
        case MPI_THREAD_SINGLE: 
            if (debug) printf("Brak wsparcia dla wątków, kończę\n");
            fprintf(stderr, "Brak wystarczającego wsparcia dla wątków - wychodzę!\n");
            MPI_Finalize();
            exit(-1);
            break;
        case MPI_THREAD_FUNNELED: 
            if (debug) printf("tylko te wątki, ktore wykonaly mpi_init_thread mogą wykonać wołania do biblioteki mpi\n");
	        break;
        case MPI_THREAD_SERIALIZED: 
            if (debug) printf("tylko jeden watek naraz może wykonać wołania do biblioteki MPI\n");
	        break;
        case MPI_THREAD_MULTIPLE:
            if (debug) printf("Pełne wsparcie dla wątków\n");
	        break;
        default:
            if (debug) printf("Nikt nic nie wie\n");
    }
}

int main(int argc, char **argv) {
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    check_thread_support(provided);

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
    srand(time(NULL) * tid);

    pthread_create( &threadCom, NULL, startCommunicationThread , 0);
}

void mainLoop() {
    process = Process();
    Packet *packet = new Packet();

    while(1) {
        // losuję śmieci
        process.drawTrashes();
        packet->data = process.getTrashes();
        printDebugInfo("Wylosowałem " + to_string(process.getTrashes()) + " pokoi", CYN);
        
        // żądam pokoi - zmniejszamy lokalnie zasób i wysyłamy żądanie
        reduceRoom(process.getTrashes());
        requestResource(State::WAIT_ACK_ROOM, Message::REQ_ROOM, "Żądam pokoi", packet);
        
        // żądam wind - zmniejszamy lokalnie zasób i wysyłamy żądanie
        reduceElev();
        requestResource(State::WAIT_ACK_ELEV, Message::REQ_ELEV, "Żądam windę na dół", packet);
        
        // dostałem oba zasoby - można zjeżdżać na dół
        changeState(State::IN_ELEV);
        printDebugInfo(">SK< Jadę windą na dół", GRN);
        sleep(randTime(5));
        printDebugInfo("<SK> Zjechałem na dół", GRN);

        // jestem na dole - wchodzimy do pokoju i zwalniamy windę
        changeState(State::IN_ROOM);
        beforeReleaseElev();
        sendToAll(packet, Message::REL_ELEV);
        printDebugInfo(">SK< Zajmuję pomieszczenie i wyrzucam śmieci", BLU);
        sleep(randTime(5));
        printDebugInfo("<SK> Wyrzuciłem śmieci", BLU);
        
        // wyrzuciłem śmieci - można wracać - ządam windy - zmniejszamy lokalnie zasób i wysyłamy żądanie
        reduceElev();
        requestResource(State::WAIT_ACK_ELEV_BACK, Message::REQ_ELEV, "Żądam windę na górę", packet);
        
        // jestem w windzie, mogę bezpiecznie zwolnić pokoje
        changeState(State::IN_ELEV_BACK);
        beforeReleaseRoom(process.getTrashes());
        sendToAll(packet, Message::REL_ROOM);
        printDebugInfo(">SK< Wjeżdżam windą na górę", YEL);
        sleep(randTime(5));
        printDebugInfo("<SK> Wjechałem na górę", YEL);

        // wjechałem na górę - mogę bezpiecznie zwolnić windę
        beforeReleaseElev();
        sendToAll(packet, Message::REL_ELEV);
    }
    changeState(State::END);       
}

void finalize() {
    pthread_join(threadCom, NULL);
    printDebugInfo("Communication thread joined", RED);
    MPI_Type_free(&MPI_PACKET_T);
    MPI_Finalize();
}

void reduceRoom(int val) {
    pthread_mutex_lock( &resourceMut );
    process.decreaseHeadRoom(val);
    pthread_mutex_unlock( &resourceMut );
}

void reduceElev() {
    pthread_mutex_lock( &resourceMut );
    process.decrementHeadElev();
    pthread_mutex_unlock( &resourceMut );
}

// zanim uwolnimy zasoby, należy lokalnie zwiększyć to co wcześniej odjęliśmy oraz ustawić
// wartość head tak, żeby się zgadzała z zapamietanymi żądaniami (tail)
// potencjalnie jesteśmy ostatni w kolejce, więc ustawiamy wartość tail na 0
void beforeReleaseElev() {
    pthread_mutex_lock( &resourceMut );
    process.incrementHeadElev();
    process.setHeadElev(process.getHeadElev() - process.getTailElev());
    process.setTailElev(0);
    pthread_mutex_unlock( &resourceMut );
}

void beforeReleaseRoom(int val) {
    pthread_mutex_lock( &resourceMut );
    process.increaseHeadRoom(val);
    process.setHeadRoom(process.getHeadRoom() - process.getTailRoom());
    process.setTailRoom(0);
    pthread_mutex_unlock( &resourceMut );
}

void requestResource(State ackState, Message tag, string debugInfo, Packet* packet) {
    printDebugInfo(debugInfo);
    changeState(ackState);
    sendToAll(packet, tag);
    pthread_mutex_lock(&ackMut);
    if (!process.getCanProceed())
        threadWait();
    process.setCanProceed(false);
    pthread_mutex_unlock(&ackMut);
}

void sendPacket(Packet *packet, int destination, int tag) {
    pthread_mutex_lock( &resourceMut );
    process.incrementTimeStamp();  
    pthread_mutex_unlock( &resourceMut );
    packet->src = tid;
    packet->ts = process.getTimeStamp();
    MPI_Send(packet, 1, MPI_PACKET_T, destination, tag, MPI_COMM_WORLD);
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

void ackReceived(int ts) {
    pthread_mutex_lock( &resourceMut );
    process.incrementAckCounter();
    setTimeStamp(ts);
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
    pthread_mutex_unlock( &resourceMut );
}

// gdy dostaniemy wszystkie ackna postawie obecnego stanu ustawiamy wartość jakich 
// zasobów należy sprawdzić aby móc procedować, a także do jakiego stanu należy przejść
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
        if (tid != i)
            sendPacket(packet, i, tag);
}

void printDebugInfo(string msg, string col) {
    cout << col << "[" << tid << "] " << "[" << process.getTimeStamp() << "] ";
    cout << msg << " | HEAD=" << process.getHeadRoom() << ", TAIL=" << process.getTailRoom() << RESET << endl;
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
    process.setCanProceed(true);
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
            switch (state) {
                case INIT:
                case IN_ELEV_BACK:
                    process.decreaseHeadRoom(data);
                    break;
                case WAIT_ACK_ROOM:
                    if (process.getTimeStamp() < ts || (process.getTimeStamp() == ts && tid < src)){ // it means that incoming packet is in front of this process in Q
                        process.increaseTailRoom(data);
                    }
                    else {
                        process.decreaseHeadRoom(data);   
                    }
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
            setTimeStamp(ts);
            sendAck(src);
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
                    if (process.getTimeStamp() < ts || (process.getTimeStamp() == ts && tid < src)) // it means that incoming packet is in front of this process in Q
                        process.incrementTailElev();
                    else
                        process.decrementHeadElev();
                    break;
                case WAIT_ELEV:
                case IN_ELEV:
                case WAIT_ELEV_BACK:
                case IN_ELEV_BACK:
                    process.incrementTailElev();
                    break;
            }
            setTimeStamp(ts);
            sendAck(src);
            break;

        case Message::REL_ROOM:
            process.increaseHeadRoom(data);
            if (state == WAIT_ROOM) 
                if (process.getHeadRoom() >= 0)
                    threadWake();
            setTimeStamp(ts);
            break;

        case Message::REL_ELEV:
            process.incrementHeadElev();
            if (state == WAIT_ELEV || state == WAIT_ELEV_BACK)
                if (process.getHeadElev() >= 0)
                    threadWake();
            setTimeStamp(ts);
            break;
    }
    pthread_mutex_unlock( &resourceMut );
}
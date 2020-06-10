#include "communicationThread.h"
#include "main.h"

void *startCommunicationThread(void *ptr) {
    MPI_Status status;
    Packet packet;

    while (state != END) {
        MPI_Recv( &packet, 1, MPI_PACKET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        switch (status.MPI_TAG) {
            case Message::ACK:
                ackReceived();
                break;

            case Message::REQ_ELEV:
            case Message::REQ_ROOM:
            case Message::RES_ELEV:
            case Message::RES_ROOM:
                changeResources(status.MPI_TAG, packet);
                break;

            case Message::FINISH:
                changeState(END);
                break;
        }
    }
}
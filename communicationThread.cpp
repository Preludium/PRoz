#include "communicationThread.h"
#include "main.h"

void *startCommunicationThread(void *ptr) {
    MPI_Status status;
    packet_t packet;

    while (state != Finish) { // TODO: add state when process requests nothing

        MPI_Recv( &packet, 1, MPI_PACKET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        switch (status.MPI_TAG) {
            case Message::ACK:
                ackReceived();
                break;

            case Message::REQ_ELEV:
                changeResources(status.MPI_TAG, packet.data, packet.ts);
                break;

            case Message::REQ_ROOM:
                changeResources(status.MPI_TAG, packet.data, packet.ts);
                break;

            case Message::RES_ELEV:
                changeResources(status.MPI_TAG, packet.data, packet.ts);
                break;

            case Message::RES_ROOM:
                changeResources(status.MPI_TAG, packet.data, packet.ts);
                break;

            case Message::FINISH:
                changeState(Finish);
                break;

            default:
                break;
        }
    }
}
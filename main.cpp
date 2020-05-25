#include "main.h"

MPI_Datatype MPI_PACKAGE_T;
MPI_Status status;
pthread_mutex_t lock;

int size, rank;


void init();
void mainLoop();
void finalize();

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

    MPI_Type_create_struct(nitems, blocklengths, offsets, types, &MPI_PACKAGE_T);
    MPI_Type_commit(&MPI_PACKAGE_T);

    MPI_Comm_size( MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
}

void mainLoop() {

}

void finalize() {

}
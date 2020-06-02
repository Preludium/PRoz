#ifndef MAINH
#define MAINH

// #include <iostream>
// #include <cstdlib>
#include <mpi.h>
#include <unistd.h>
#include <pthread.h>
// #include "process.h"
// using namespace std;


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

#define MSG_SIZE 64

typedef struct { // struktura requestow
    int ts;
    int src;
    int data;
} packet_t;

enum message {ACK, REQ_ELEV, REQ_ROOM, RES_ELEV, RES_ROOM};

#endif 
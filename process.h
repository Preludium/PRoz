#ifndef PROCESSH
#define PROCESSH

#include <iostream>
#include <cstdlib>
using namespace std;

class Process {
    int headElev, headRoom, tailElev, tailRoom, trashes, ackCounter;
public:
    Process();
    Process(int headElev, int headRoom);
    int getHeadElev();
    void setHeadElev(int headElev);
    int getHeadRoom();
    void setHeadRoom(int headRoom);
    int getTailElev();
    void setTailElev(int tailElev);
    int getTailRoom();
    void setTailRoom(int tailRoom);
    int getTrashes();
    void setTrashes(int trashes);
    int getAckCounter();
    void setAckCounter(int num);
    void incrementHeadElev();
    void incrementTailElev();
    void decrementHeadElev();
    void decrementTailElev();
    void increaseHeadRoom(int rooms);
    void increaseTailRoom(int rooms);
    void decreaseHeadRoom(int rooms);
    void decreaseTailRoom(int rooms);
};

#endif
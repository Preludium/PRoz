#include "process.h"

Process::Process() {

}

Process::Process(int headElev, int headRoom) {
    this->headElev = headElev;
    this->headRoom = headRoom;
    this->tailElev = 0;
    this->tailRoom = 0;
    this->trashes = 1 + rand() % (headRoom - 1);
    this->ackCounter = 0;
}

int Process::getHeadElev() {
    return this->headElev;
}

void Process::setHeadElev(int headElev) {
    this->headElev = headElev;
}

int Process::getHeadRoom() {
    return this->headRoom;
}

void Process::setHeadRoom(int headRoom) {
    this->headRoom = headRoom;
}

int Process::getTailElev() {
    return this->tailElev;
}

void Process::setTailElev(int tailElev) {
    this->tailElev = tailElev;
}

int Process::getTailRoom() {
    return this->tailRoom;
}

void Process::setTailRoom(int tailRoom) {
    this->tailRoom = tailRoom;
}

int Process::getTrashes() {
    return this->trashes;
}

void Process::setTrashes(int trashes) {
    this->trashes = trashes;
}

int Process::getAckCounter() {
    return this->ackCounter;
}

void Process::setAckCounter(int num) {
    this->ackCounter = num;
}

void Process::incrementHeadElev() {
    this->headElev++;
}

void Process::incrementTailElev() {
    this->tailElev++;
}

void Process::decrementHeadElev() {
    this->headElev--;
}

void Process::decrementTailElev() {
    this->tailElev--;
}

void Process::increaseHeadRoom(int rooms) {
    this->headRoom += rooms;
}

void Process::increaseTailRoom(int rooms) {
    this->tailRoom += rooms;
}

void Process::decreaseHeadRoom(int rooms) {
    this->headRoom -= rooms;
}

void Process::decreaseTailRoom(int rooms) {
    this->tailRoom -= rooms;
}
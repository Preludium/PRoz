#include "process.h"
#include "main.h"

Process::Process() {
    this->headElev = MAX_ELEV;
    this->headRoom = MAX_ROOM;
    this->tailElev = 0;
    this->tailRoom = 0;
    this->trashes = 0;
    this->ackCounter = 0;
    this->timeStamp = 0;
    this->canProceed = false;
    this->requestMoment = 0;
}

void Process::setRequestMoment(int requestMoment) {
    this->requestMoment = requestMoment;
}

int Process::getRequestMoment() {
    return this->requestMoment;
}

void Process::drawTrashes() {
    this->trashes = 1 + rand() % (MAX_ROOM - 1);
}

bool Process::getCanProceed() {
    return this->canProceed;
}

void Process::setCanProceed(bool canProceed) {
    this->canProceed = canProceed;
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

void Process::setAckCounter(int ackCounter) {
    this->ackCounter = ackCounter;
}

int Process::getTimeStamp() {
    return this->timeStamp;
}

void Process::setTimeStamp(int timeStamp) {
    this->timeStamp = timeStamp;
}

void Process::incrementTimeStamp() {
    this->timeStamp++;
}

void Process::incrementAckCounter() {
    this->ackCounter++;
}

void Process::clearAckCounter() {
    this->ackCounter = 0;
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
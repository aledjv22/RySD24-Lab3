#ifndef QUEUE
#define QUEUE

#include <string.h>
#include <omnetpp.h>

using namespace omnetpp;

class Queue: public cSimpleModule {
private:
    cQueue buffer;
    cMessage *endServiceEvent;
    simtime_t serviceTime;
    cOutVector bufferSizeVector;
    cOutVector packetDropVector;
public:
    Queue();
    virtual ~Queue();
protected:
    virtual void initialize();
    virtual void finish();
    virtual void handleMessage(cMessage *msg);
};

Define_Module(Queue);

Queue::Queue() {
    endServiceEvent = NULL;
}

Queue::~Queue() {
    cancelAndDelete(endServiceEvent);
}

void Queue::initialize() {
    buffer.setName("buffer");
    endServiceEvent = new cMessage("endService");
}

void Queue::finish() {
}

void Queue::handleMessage(cMessage *msg) {
    // check buffer limit
    if (buffer.getLength() >= par("BufferSize").longValue()) {
        // drop the packet
        delete msg;
        this->buble("packet dropped");
        packetDropVector.record(1);
    } else {
        // enqueue the packet
        buffer.insert(msg);
        bufferSizeVector.record(buffer.getLength());
        // if the server is idle
        if(!endServiceEvent->isScheduled()){
            // start the service now
            scheduleAt(simTime() + 0, endServiceEvent);
        }
    }
}

#endif /* QUEUE */

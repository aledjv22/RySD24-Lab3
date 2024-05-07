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
    // Objects of type cOutVector are responsible
    // for writing time series data (referred to as
    // output vectors) to a file. The record()
    // method is used to output a value (or a value pair)
    // with a timestamp. The object name will serve as
    // the name of the output vector.
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
    bufferSizeVector.setName("Size-Vector");
    packetDropVector.setName("Drop-Vector");
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

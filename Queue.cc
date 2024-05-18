#ifndef QUEUE
#define QUEUE

#include <string.h>
#include <omnetpp.h>

using namespace omnetpp;

class Queue: public cSimpleModule
{
private:
    cQueue buffer;
    cMessage *endServiceEvent;
    simtime_t serviceTime;
    bool feedbackSent;
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

Queue::Queue()
{
    endServiceEvent = NULL;
}

Queue::~Queue()
{
    cancelAndDelete(endServiceEvent);
}

void Queue::initialize()
{
    buffer.setName("buffer");
    endServiceEvent = new cMessage("endService");
    bufferSizeVector.setName("Size-Vector");
    packetDropVector.setName("Drop-Vector");
    feedbackSent = false;
}

void Queue::finish()
{
}

void Queue::handleMessage(cMessage *msg)
{
    if(msg == endServiceEvent)
    {
        if (!buffer.isEmpty())
        {
            cPacket *pkt = (cPacket *)buffer.pop();
            send(pkt, "out");
            serviceTime = pkt->getDuration();
            scheduleAt(simTime() + serviceTime, endServiceEvent);
            bufferSizeVector.record(buffer.getLength());
        }
    }
    else
    {
        // check buffer limit
        if (buffer.getLength() >= par("bufferSize").intValue())
        {
            // Changed longValue to intValue due to it not existing
            // drop the packet
            delete msg;
            this->bubble("packet dropped");
            packetDropVector.record(1);
        }
        else
        {
            float umbral = 0.80 * par("bufferSize").intValue();
            float umbralMin = 0.25 * par("bufferSize").intValue();

            if (buffer.getLength() >= umbral && !feedbackSent)
            {
                cPacket *feedbackPkt = new cPacket("packet");
                feedbackPkt->setByteLength(20);
                feedbackPkt->setKind(2);
                buffer.insertBefore(buffer.front(), feedbackPkt);
                feedbackSent = true;
            }
            else if (buffer.getLength() < umbralMin && feedbackSent)
            {
                cPacket *feedbackPkt = new cPacket("packet");
                feedbackPkt->setByteLength(20);
                feedbackPkt->setKind(3);
                buffer.insertBefore(buffer.front(), feedbackPkt);
                feedbackSent = false;
            }
            // Enqueue the packet
            buffer.insert(msg);
            bufferSizeVector.record(buffer.getLength());
            // if the server is idle
            if (!endServiceEvent->isScheduled())
            {
                // start the service
                scheduleAt(simTime() + 0, endServiceEvent);
            }
        }
    }
}

#endif /* QUEUE */

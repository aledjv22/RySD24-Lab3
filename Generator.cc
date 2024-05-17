#ifndef GENERATOR
#define GENERATOR

#include <string.h>
#include <omnetpp.h>

using namespace omnetpp;

class Generator : public cSimpleModule
{
private:
    cMessage *sendMsgEvent;
    cStdDev transmissionStats;
    unsigned int sentPackets;
public:
    Generator();
    virtual ~Generator();
protected:
    virtual void initialize();
    virtual void finish();
    virtual void handleMessage(cMessage *msg);
};
Define_Module(Generator);

Generator::Generator()
{
    sendMsgEvent = NULL;

}

Generator::~Generator()
{
    cancelAndDelete(sendMsgEvent);
}

void Generator::initialize()
{
    transmissionStats.setName("TotalTransmissions");
    // create the send packet
    sendMsgEvent = new cMessage("sendEvent");
    // schedule the first event at random time
    scheduleAt(par("generationInterval"), sendMsgEvent);
    sentPackets = 0;
}

void Generator::finish()
{
    recordScalar("Packages sent", sentPackets);
}

void Generator::handleMessage(cMessage *msg)
{

    // create new packet
    cPacket *pkt = new cPacket("packet");
    // The cPacket constructor is similar
    // to the cMessage constructor, but it
    // accepts an additional bit length argument

    pkt->setByteLength(par("packetByteSize"));
    // send to the output
    send(pkt, "out");
    sentPackets++;
    // compute the new departure time
    simtime_t departureTime = simTime() + par("generationInterval");
    // schedule the new packet generation
    scheduleAt(departureTime, sendMsgEvent);
}

#endif /* GENERATOR */

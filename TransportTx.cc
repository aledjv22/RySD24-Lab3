#ifndef TRANSPORT_TX
#define TRANSPORT_TX

#include <omnetpp.h>
#include <string.h>

using namespace omnetpp;

class TransportTx : public cSimpleModule
{
private:
    cOutVector bufferSizeQueue;
    unsigned int packetDropQueue;
    cQueue bufferPackets;
    cMessage *serviceEndEvent;
    simtime_t serviceTime;
    float packetRate;
public:
    TransportTx();
    virtual ~TransportTx();
protected:
    virtual void initialize();
    virtual void finish();
    virtual void handleMessage(cMessage *msg);
};

Define_Module(TransportTx);

TransportTx::TransportTx()
{
    serviceEndEvent = NULL;
}

TransportTx::~TransportTx()
{
    cancelAndDelete(serviceEndEvent);
}

void TransportTx::initialize()
{
    bufferPackets.setName("Buffer_del_transmisor");
    bufferSizeQueue.setName("Tamaño_del_buffer");
    packetDropQueue = 0;
    serviceEndEvent = new cMessage("Fin_Servicio");
    packetRate = 1.0;
}

void TransportTx::finish()
{
    recordScalar("Paquetes_descartados", packetDropQueue);
}

void TransportTx::handleMessage(cMessage *msg)
{
    bufferSizeQueue.record(bufferPackets.getLength());
    if (msg == serviceEndEvent)
    {
        if (!bufferPackets.isEmpty())
        {
            cPacket *pkt = (cPacket*) bufferPackets.pop();
            send(pkt, "toOut$o");
            serviceTime = pkt->getDuration();
            scheduleAt(simTime() + serviceTime*packetRate, serviceEndEvent);
        }
    }
    else
    {
        if (bufferPackets.getLength() >= par("Tamaño_Buffer").intValue())
        {
            delete(msg);
            this->bubble("Paquete_descartado");
            packetDropQueue++;
        }
        else
        {
            if (msg->getKind() == 2)
            {
                packetRate = packetRate*2;
            }
            else if (msg->getKind() == 3)
            {
                packetRate = packetRate/2;
            }
            else
            {
                bufferPackets.insert(msg);
                bufferSizeQueue.record(bufferPackets.getLength());
                if (!serviceEndEvent->isScheduled())
                {
                    scheduleAt(simTime() + 0, serviceEndEvent);
                }
            }
        }
    }
}

#endif

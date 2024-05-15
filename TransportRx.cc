#ifndef TRANSPORT_RX
#define TRANSPORT_RX

#include <omnetpp.h>
#include <string.h>

using namespace omnetpp;

class TransportRx: public cSimpleModule {
    private:
        cOutVector bufferSizeQueue;
        cOutVector packetDropQueue;
        cQueue bufferPackets;
        cQueue feedbackQueue;
        cMessage *serviceEndEvent;
        cMessage *feedbackEndEvent;
        simtime_t serviceTime;
        bool feedbackSent;

    public:
        TransportRx();
        virtual ~TransportRx();
    protected:
        virtual void initialize();
        virtual void finish();
        virtual void handleMessage(cMessage *msg);

};

Define_Module(TransportRx);

TransportRx::TransportRx() {
    serviceEndEvent = NULL;
    feedbackEndEvent = NULL;
}

TransportRx::~TransportRx() {
    cancelAndDelete(serviceEndEvent);
    cancelAndDelete(feedbackEndEvent);
}

void TransportRx::initialize() {
    bufferPackets.setName("Buffer del receptor");
    bufferSizeQueue.setName("Tamaño del buffer");
    packetDropQueue.setName("Paquetes descartados");
    feedbackQueue.setName("Buffer de retroalimentación");
    packetDropQueue.record(0);
    serviceEndEvent = new cMessage("Fin Servicio");
    feedbackEndEvent = new cMessage("Fin Retroalimentación");
    feedbackSent = false;
}

void TransportRx::finish() {
    recordScalar("Paquetes_decartados", packetDropQueue.getCount());
}

void TransportRx::handleMessage(cMessage *msg) {
    bufferSizeQueue.record(bufferPackets.getLength());
    if (msg == serviceEndEvent) {
        if (!bufferPackets.isEmpty()) {
            cPacket *pkt = (cPacket*) bufferPackets.pop();
            send(pkt, "toApp");
            serviceTime = pkt->getDuration();
            scheduleAt(simTime() + serviceTime, serviceEndEvent);
        }
    } else if (msg == feedbackEndEvent) {
        if (!feedbackQueue.isEmpty()) {
            cPacket *pkt = (cPacket*) feedbackQueue.pop();
            send(pkt, "toOut$o");
            scheduleAt(simTime() + pkt->getDuration(), feedbackEndEvent);
        }
    } else {
        if (bufferPackets.getLength() >= par("Tamaño_Buffer").intValue()) {
            delete(msg);
            this->bubble("Paquete_descartado");
            packetDropQueue.record(1);
        } else {
            if (msg->getKind() == 2 || msg->getKind() == 3){
                feedbackQueue.insert(msg);
                if (!feedbackEndEvent->isScheduled()) {
                    scheduleAt(simTime() + 0, feedbackEndEvent);
                }
            } else {
                float umbral = 0.80 * par("Tamaño_Buffer").intValue();
                float umbralMin = 0.25 * par("Tamaño_Buffer").intValue();
                if (bufferPackets.getLength() >= umbral && !feedbackSent){
                    cPacket *feedbackPkt = new cPacket("Paquete");
                    feedbackPkt->setByteLength(20);
                    feedbackPkt->setKind(2);
                    send(feedbackPkt, "toOut$o");
                    feedbackSent = true;
                }else if (bufferPackets.getLength() < umbralMin && feedbackSent){
                    cPacket *feedbackPkt = new cPacket("Paquete");
                    feedbackPkt->setByteLength(20);
                    feedbackPkt->setKind(3);
                    send(feedbackPkt, "toOut$o");
                    feedbackSent = false;
                }
                bufferPackets.insert(msg);
                bufferSizeQueue.record(bufferPackets.getLength());
                if (!serviceEndEvent->isScheduled()) {
                    scheduleAt(simTime() + 0, serviceEndEvent);
                }
            }
        }
    }
}

#endif
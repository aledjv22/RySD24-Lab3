#ifndef TRANSPORT_RX
#define TRANSPORT_RX

#include <omnetpp.h>
#include <string.h>

using namespace omnetpp;

class TransportRx: public cSimpleModule {
    private:
        cQueue bufferPackets; 
        cQueue feedbackQueue; 
        cMessage *serviceEndEvent; 
        cMessage *feedbackEndEvent; 
        cOutVector packetBufferSizeVec; 
        cOutVector packetDropVec; 
        int packetDropCounter; 

        void transmitPacket(); 
        void transmitFeedback(); 
        void addFeedbackToQueue(cMessage *msg); 
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
    feedbackQueue.setName("Buffer de retroalimentación");    
    packetBufferSizeVec.setName("Tamaño del buffer");
    packetDropVec.setName("Paquetes descartados");
    packetDropCounter = 0;
    serviceEndEvent = new cMessage("Fin Servicio");
    feedbackEndEvent = new cMessage("Fin Retroalimentación");
}

void TransportRx::finish() {
    recordScalar("Paquetes decartados", packetDropCounter);
}

void TransportRx::transmitPacket() {
    if (!bufferPackets.isEmpty()) {
        // si hay paquetes en el buffer, envía el siguiente
        cPacket *pkt = (cPacket*) bufferPackets.pop();
        send(pkt, "toApp");
        scheduleAt(simTime() + pkt->getDuration(), serviceEndEvent);
    }
}

void TransportRx::addFeedbackToQueue(cMessage *msg){
    if (feedbackQueue.getLength() < par("Tamaño del búfer de retroalimentación").intValue()) {
        feedbackQueue.insert(msg);
        if (!feedbackEndEvent->isScheduled()) {
            scheduleAt(simTime() + 0, feedbackEndEvent);
        }
    } else {
        delete msg;
    }
}

#endif
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

void TransportRx::transmitFeedback() {
    if (!feedbackQueue.isEmpty()) {
        // si el buffer de retroalimentación no está vacío, envía el siguiente
        FeedbackPkt *pkt = (FeedbackPkt*) feedbackQueue.pop();
        send(pkt, "toOut$o");
        scheduleAt(simTime() + pkt->getDuration(), feedbackEndEvent);
    }
}

void TransportRx::handleMessage(cMessage *msg) {
    packetBufferSizeVec.record(bufferPackets.getLength());
    if (msg == serviceEndEvent) {
        // el mensaje es serviceEndEvent
        transmitPacket();    
    } else {
        // encola el mensaje
        if (msg->getKind() == 2) {
            const int threshold = 0.70 * par("Tamaño Buffer").intValue();
            if (bufferPackets.getLength() < par("Tamaño Buffer").intValue()) {
                if (bufferPackets.getLength() >= threshold){
                    FeedbackPkt *fPkt = new FeedbackPkt();
                    fPkt->setkind(2);
                    fPkt->setByteLength(1);
                    addFeedbackToQueue(fPkt);                
                }
                bufferPackets.insert(msg);
                if (!serviceEndEvent->isScheduled()) {
                    scheduleAt(simTime() + 0, serviceEndEvent);
                }
            } else {
                this->bubble("Paquete descartado");
                packetDropCounter++;
                packetDropVec.record(packetDropCounter);
                delete msg;
            }
        } else{
            // el mensaje es un paquete de retroalimentación
            addFeedbackToQueue(msg);
        }
    }
}
#endif
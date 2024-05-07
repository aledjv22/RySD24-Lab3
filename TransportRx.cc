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
#endif
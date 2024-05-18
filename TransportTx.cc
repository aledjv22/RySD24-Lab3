#ifndef TRANSPORT_TX
#define TRANSPORT_TX  // This line prevents multiple inclusions of the header file

#include <omnetpp.h>
#include <string.h>  // Not strictly necessary in this code

using namespace omnetpp;

class TransportTx : public cSimpleModule
{
private:
    // Module variables to store data
    cOutVector bufferSizeQueue;  // Vector to record buffer size over time
    unsigned int packetDropQueue;  // Counter for dropped packets
    cQueue bufferPackets;        // Queue to store packets waiting for transmission
    cMessage *serviceEndEvent;   // Message used to schedule service completion
    simtime_t serviceTime;      // Time required to transmit a packet
    float packetRate;            // Rate at which packets are generated (initially 1.0)

public:
    TransportTx();
    virtual ~TransportTx();

protected:
    virtual void initialize();
    virtual void finish();
    virtual void handleMessage(cMessage *msg);
};

Define_Module(TransportTx);  // Macro to register TransportTx as a module

// Implementations of the member functions

TransportTx::TransportTx()
{
    serviceEndEvent = NULL;  // Initialize serviceEndEvent to NULL
}

TransportTx::~TransportTx()
{
    cancelAndDelete(serviceEndEvent);  // Cancel and delete the serviceEndEvent message
}

void TransportTx::initialize()
{
    bufferPackets.setName("Buffer_del_transmisor");  // Set name for the buffer queue
    bufferSizeQueue.setName("bufferSize");         // Set name for the buffer size recording vector
    packetDropQueue = 0;                           // Initialize packet drop counter to 0
    serviceEndEvent = new cMessage("Fin_Servicio");  // Create a new message "Fin_Servicio"
    packetRate = 1.0;                             // Set initial packet rate to 1.0
}

void TransportTx::finish()
{
    recordScalar("Paquetes_descartados", packetDropQueue);  // Record the number of dropped packets
}

void TransportTx::handleMessage(cMessage *msg)
{
    // Record current buffer size
    bufferSizeQueue.record(bufferPackets.getLength());

    if (msg == serviceEndEvent)
    {
        // Service completion event received
        if (!bufferPackets.isEmpty())
        {
            // If there are packets in the buffer
            cPacket *pkt = (cPacket*)bufferPackets.pop();  // Get the first packet
            send(pkt, "toOut$o");                         // Send the packet out
            serviceTime = pkt->getDuration();             // Get the packet's transmission time
            scheduleAt(simTime() + serviceTime * packetRate, serviceEndEvent);  // Schedule next service event
        }
    }
    else
    {
        // Received a new packet (not service completion event)
        if (bufferPackets.getLength() >= par("bufferSize").intValue())
        {
            // Buffer is full, drop the packet
            delete(msg);
            this->bubble("Paquete_descartado");  // Display a bubble notification
            packetDropQueue++;
        }
        else
        {
            // Buffer has space
            if (msg->getKind() == 2)
            {
                // Packet kind 2: increase packet rate
                packetRate = packetRate * 2;
            }
            else if (msg->getKind() == 3)
            {
                // Packet kind 3: decrease packet rate
                packetRate = packetRate / 2;
            }
            else
            {
                // Regular packet: store in the buffer
                bufferPackets.insert(msg);
                bufferSizeQueue.record(bufferPackets.getLength());  // Record new buffer size
                if (!serviceEndEvent->isScheduled())
                {
                    // Schedule the first service event if not already scheduled
                    scheduleAt(simTime() + 0, serviceEndEvent);
                }
            }
        }
    }
}

#endif

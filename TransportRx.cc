#ifndef TRANSPORT_RX
#define TRANSPORT_RX  // This line prevents multiple inclusions of the header file

#include <omnetpp.h>
#include <string.h>  // Not strictly necessary in this code

using namespace omnetpp;

class TransportRx : public cSimpleModule
{
private:
    // Module variables to store data
    cOutVector bufferSizeQueue;   // Vector to record buffer size over time
    cOutVector packetDropQueue;  // Vector to record dropped packets
    cQueue buffer;               // Queue to store received packets
    cQueue feedbackBuffer;       // Queue to store feedback packets to be sent
    cMessage *endServiceEvent;  // Message used to schedule service completion
    cMessage *endFeedbackEvent; // Message used to schedule feedback transmission
    simtime_t serviceTime;       // Time required to process a received packet
    bool feedbackSent;          // Flag to indicate if feedback has been recently sent

public:
    TransportRx();
    virtual ~TransportRx();

protected:
    virtual void initialize();
    virtual void finish();
    virtual void handleMessage(cMessage *msg);
};

Define_Module(TransportRx);  // Macro to register TransportRx as a module

// Implementations of the member functions

TransportRx::TransportRx()
{
    endServiceEvent = NULL;
    endFeedbackEvent = NULL;
}

TransportRx::~TransportRx()
{
    cancelAndDelete(endServiceEvent);
    cancelAndDelete(endFeedbackEvent);
}

void TransportRx::initialize()
{
    buffer.setName("buffer");           // Set name for the packet buffer queue
    bufferSizeQueue.setName("BufferSizeQueue");  // Set name for the buffer size recording vector
    packetDropQueue.setName("PacketDropQueue");  // Set name for the dropped packet recording vector
    feedbackBuffer.setName("bufferFeedback");  // Set name for the feedback packet buffer
    packetDropQueue.record(0);              // Initialize dropped packet counter to 0
    endServiceEvent = new cMessage("endService");  // Create a new message "endService"
    endFeedbackEvent = new cMessage("endFeedback");  // Create a new message "endFeedback"
    feedbackSent = false;                 // Initialize feedbackSent flag to false
}

void TransportRx::finish()
{
    // No specific actions required in finish() for this module
}

void TransportRx::handleMessage(cMessage *msg)
{
    // Check if the message is the endServiceEvent
    if (msg == endServiceEvent)
    {
        // Service completion event received
        if (!buffer.isEmpty())
        {
            // If there are packets in the buffer
            cPacket *pkt = (cPacket*)buffer.pop();  // Get the first packet
            send(pkt, "toApp");                   // Send the packet to the application layer
            serviceTime = pkt->getDuration();     // Get the packet's processing time
            scheduleAt(simTime() + serviceTime, endServiceEvent);  // Schedule the next service event
        }
    }
    else if (msg == endFeedbackEvent)
    {
        // The message is the endFeedbackEvent
        if (!feedbackBuffer.isEmpty())
        {
            // If there are feedback packets to send
            cPacket *pkt = (cPacket*)feedbackBuffer.pop();  // Get the first feedback packet
            send(pkt, "toOut$o");                           // Send the feedback packet
            scheduleAt(simTime() + pkt->getDuration(), endFeedbackEvent);  // Schedule the next feedback transmission
        }
    }
    else     // The message is a received data packet
    {
        if (buffer.getLength() >= par("bufferSize").intValue())
        {
            // Buffer is full, drop the packet
            delete(msg);
            this->bubble("packet-dropped");
            packetDropQueue.record(1);
        }
        else
        {
            // Buffer has space
            if (msg->getKind() == 2 || msg->getKind() == 3)
            {
                // Packet is a feedback packet (kind 2 or 3)
                feedbackBuffer.insert(msg);  // Enqueue the feedback packet

                if (!endFeedbackEvent->isScheduled())
                {
                    // If no feedback transmission is scheduled, start it now
                    scheduleAt(simTime() + 0, endFeedbackEvent);
                }
            }
            else
            {
                // Regular data packet
                float threshold = 0.80 * par("bufferSize").intValue();  // High threshold for sending feedback (80% of buffer
                float thresholdMin = 0.25 * par("bufferSize").intValue();  // Low threshold (25% of buffer size)
                // Logic for sending feedback based on buffer occupancy and feedbackSent flag
                if (buffer.getLength() >= threshold && !feedbackSent)
                {
                    // Buffer occupancy is high and no recent feedback sent
                    cPacket *feedbackPkt = new cPacket("packet");  // Create a new feedback packet
                    feedbackPkt->setByteLength(20);                 // Set the feedback packet size
                    feedbackPkt->setKind(2);                        // Set the feedback packet kind to 2 (requesting rate decrease)
                    send(feedbackPkt, "toOut$o");                  // Send the feedback packet
                    feedbackSent = true;                           // Update feedbackSent flag
                }
                else if (buffer.getLength() < thresholdMin && feedbackSent)
                {
                    // Buffer occupancy is low and recent feedback sent (kind 2)
                    cPacket *feedbackPkt = new cPacket("packet");  // Create a new feedback packet
                    feedbackPkt->setByteLength(20);                 // Set the feedback packet size
                    feedbackPkt->setKind(3);                        // Set the feedback packet kind to 3 (requesting rate increase)
                    send(feedbackPkt, "toOut$o");                  // Send the feedback packet
                    feedbackSent = false;                          // Update feedbackSent flag
                }

                // Enqueue the received data packet
                buffer.insert(msg);
                bufferSizeQueue.record(buffer.getLength());  // Record current buffer size

                // If the service is idle, schedule the next service event
                if (!endServiceEvent->isScheduled())
                {
                    scheduleAt(simTime() + 0, endServiceEvent);
                }
            }
        }
    }
}

#endif

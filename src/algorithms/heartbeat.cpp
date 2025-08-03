#include "heartbeat.h"

HeartbeatSystem::HeartbeatSystem(int nodeId, unsigned long interval, unsigned long timeout)
{
    myNodeId = nodeId;
    heartbeatInterval = interval;
    failureTimeout = timeout;
    lastHeartbeatSent = 0;
    mySequenceNumber = 0;

    totalHeartbeatsSent = 0;
    totalHeartbeatsReceived = 0;
    totalFailuresDetected = 0;

    Serial.println(" HeartbeatSystem initialized for Node " + String(myNodeId));
    Serial.println("Heartbeat Interval: " + String(heartbeatInterval) + "ms");
    Serial.println("Failur Timeout: " + String(failureTimeout) + "ms");
}

void HeartbeatSystem::sendHeartbeat()
{
    unsigned long currentTime = getCurrentTimestamp();

    if (currentTime - lastHeartbeatSent >= heartbeatInterval)
    {
        mySequenceNumber++;
        lastHeartbeatSent = currentTime;
        totalHeartbeatsSent++;

        HeartbeatMessage msg = createHeartbeatMessage();

        Serial.println("Send Heartbeat: Node " + String(myNodeId) + " | Seq: " + String(mySequenceNumber) + " | Time: " + String(currentTime));
        Serial.println(" [SIMULATED] Broadcasting heartbeat to all nodes...");
    }
}

bool HeartbeatSystem::receiveHeartbeat(int fromNodeId, unsigned long timestamp, unsigned long sequenceNumber)
{
    if (fromNodeId == myNodeId)
    {
        return false;
    }
    unsigned long currentTime = getCurrentTimestamp();
    totalHeartbeatsReceived++;

    if (knownNodes.find(fromNodeId) == knownNodes.end())
    {
        Serial.println("NEW NODE discovered: " + String(fromNodeId));
        addNode(fromNodeId);
    }
    NodeInfo &node = knownNodes[fromNodeId];
    node.lastHeartbeatTime = currentTime;
    node.lastSequenceNumber = sequenceNumber;

    if (!node.isAlive)
    {
        node.isAlive = true;
        failedNodes.erase(fromNodeId);
        Serial.println(" NODE REVIVED: " + String(fromNodeId));
    }
}
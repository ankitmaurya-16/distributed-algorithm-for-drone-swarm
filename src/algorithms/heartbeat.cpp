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
        Serial.println(" NODE REVIVED: " + String(fromNodeId) + " is back online!");
    }
    timeoutManager.resetTimeout(node.timeoutId);
    Serial.println("Receive heartbeat: Node " + String(fromNodeId) + " | Seq: " + String(sequenceNumber) + " | Latency: " + String(currentTime - timestamp) + "ms");
    return true;
}
void HeartbeatSystem::receiveHeartbeat(const HeartbeatMessage &msg)
{
    receiveHeartbeat(msg.fromNodeId, msg.timestamp, msg.sequenceNumber);
}

void HeartbeatSystem::addNode(int nodeId)
{
    if (nodeId == myNodeId)
    {
        return;
    }
    if (knownNodes.find(nodeId) != knownNodes.end())
    {
        return;
    }

    NodeInfo newNode;
    newNode.nodeId = nodeId;
    newNode.lastHeartbeatTime = getCurrentTimestamp();
    newNode.lastSequenceNumber = 0;
    newNode.isAlive = true;
    newNode.nodeName = "Node_" + String(nodeId);

    newNode.timeoutId = timeoutManager.addTimeout(failureTimeout, newNode.nodeName);
    knownNodes[nodeId] = newNode;

    Serial.println("Added node " + String(nodeId) + " to monitoring (TimeoutID: " + String(newNode.timeoutId) + ")");
}

void HeartbeatSystem::removeNode(int nodeId)
{
    if (knownNodes.find(nodeId) != knownNodes.end())
    {
        NodeInfo &node = knownNodes[nodeId];
        timeoutManager.removeTimeout(node.timeoutId);
        knownNodes.erase(nodeId);
        failedNodes.erase((nodeId));
        Serial.println("Removed node " + String(nodeId) + " from monitoring");
    }
}

bool HeartbeatSystem::isNodeAlive(int nodeId)
{
    if (knownNodes.find(nodeId) == knownNodes.end())
    {
        return false;
    }
    return knownNodes[nodeId].isAlive;
}

std::vector<int> HeartbeatSystem::getAliveNodes()
{
    std::vector<int> aliveNodes;
    for (auto &pair : knownNodes)
    {
        if (pair.second.isAlive)
        {
            aliveNodes.push_back(pair.first);
        }
    }
    return aliveNodes;
}

std::vector<int> HeartbeatSystem::getFailedNodes()
{
    std::vector<int> failed;
    for (int nodeId : failedNodes)
    {
        failed.push_back(nodeId);
    }
    return failed;
}

std::vector<int> HeartbeatSystem::checkFailedNodes()
{
    std::vector<int> expiredTimeouts = timeoutManager.checkAllTimeouts();
    std::vector<int> newlyFailedNodes;

    for (int timeoutId : expiredTimeouts)
    {
        for (auto &pair : knownNodes)
        {
            NodeInfo &node = pair.second;
            if (node.timeoutId == timeoutId && node.isAlive)
            {
                node.isAlive = false;
                failedNodes.insert(pair.first);
                newlyFailedNodes.push_back(pair.first);
                totalFailuresDetected++;
                Serial.println("Node failed: " + String(pair.first) + " (last seen: " + String(getCurrentTimestamp() - node.lastHeartbeatTime) + "ms ago");
                break;
            }
        }
    }
    return newlyFailedNodes;
}

void HeartbeatSystem::processHeartbeats()
{
    sendHeartbeat();
    std::vector<int> newFailures = checkFailedNodes();
    if (!newFailures.empty())
    {
        Serial.println("Failure detected: " + String(newFailures.size()) + " node(s) failed");
        for (int nodeId : newFailures)
        {
            Serial.println(" -Node " + String(nodeId) + " is now Offline");
        }
    }
}

void HeartbeatSystem::printStatus()
{
    Serial.println("HeartbeatSystem Status");
    Serial.println("My node Id: " + String(myNodeId));
    Serial.println("Known nodes: " + String(knownNodes.size()));
    Serial.println("Alive nodes: " + String(getAliveNodeCount()));
    Serial.println("Failed nodes: " + String(failedNodes.size()));

    if (!failedNodes.empty())
    {
        Serial.print("Failed: ");
        for (int nodeId : failedNodes)
        {
            Serial.print(String(nodeId) + " ");
        }
        Serial.println();
    }
    Serial.println("------------------------------------------------------");
}

void HeartbeatSystem::printDetailedStatus()
{
    Serial.println("Detailed heartbeatSystem status");
    Serial.println("My Node Id: " + String(myNodeId) + " | Seq: " + String(mySequenceNumber));
    Serial.println("heartbeat interval: " + String(heartbeatInterval) + "ms");
    Serial.println("failure timeout: " + String(failureTimeout) + "ms");
    Serial.println("total known Nodes: " + String(knownNodes.size()));

    unsigned long currentTime = getCurrentTimestamp();
    for (auto &pair : knownNodes)
    {
        NodeInfo &node = pair.second;
        unsigned long timeSsinceLastHB = currentTime - node.lastHeartbeatTime;
        unsigned long remainingTime = timeoutManager.getRemainingTime(node.timeoutId);

        Serial.println("Node " + String(node.nodeId) + ": " + (node.isAlive ? "Alive" : "Failed") + " | Last HB: " + String(timeSsinceLastHB) + "ms ago" + " | Timeout in: " + String(remainingTime) + "ms" + " | Seq: " + String(node.lastSequenceNumber));
    }
    Serial.println("---------------------------------------------------------");
}

HeartbeatMessage HeartbeatSystem::createHeartbeatMessage()
{
    HeartbeatMessage msg;
    msg.fromNodeId = myNodeId;
    msg.timestamp = getCurrentTimestamp();
    msg.sequenceNumber = mySequenceNumber;
    msg.messageType = 1;
    return msg;
}

int HeartbeatSystem::getAliveNodeCount()
{
    int count = 0;
    for (auto &pair : knownNodes)
    {
        if (pair.second.isAlive)
        {
            count++;
        }
    }
    return count;
}

int HeartbeatSystem::getTotalNodesCount()
{
    return knownNodes.size();
}

void HeartbeatSystem::printStatistics()
{
    Serial.println("Heartbeat Statistics");
    Serial.println("Heartbeats sent: " + String(totalHeartbeatsSent));
    Serial.println("Heartbeats Received: " + String(totalHeartbeatsReceived));
    Serial.println("Failures detected: " + String(totalFailuresDetected));
    Serial.println("Uptime: " + String(getCurrentTimestamp()) + "ms");
    Serial.println("---------------------------------------------------");
}

void HeartbeatSystem::resetStatistics()
{
    totalHeartbeatsSent = 0;
    totalHeartbeatsReceived = 0;
    totalFailuresDetected = 0;
    Serial.println("Statistics reset");
}